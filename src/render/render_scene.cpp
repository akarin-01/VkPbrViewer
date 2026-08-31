#include "render_scene.h"

#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_manager.h"
#include "resource/resource_manager.h"
#include "resource/gpu_layouts.h"
#include "scene/scene.h"

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Render
    {
        RenderScene::RenderScene(const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            Resource::DescriptorManager& descriptorMgr,
            Resource::ResourceManager& resourceMgr)
            : m_context(context),
            m_resourceMgr(resourceMgr),
            m_descriptorMgr(descriptorMgr),
            m_frameData(context, resources, swapChain, descriptorMgr)
        {
            Recreate(swapChain.Extent());
            m_object = CreateObject();
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;

            UpdateObject(m_object, frameIndex, scene.GetObject());
            UpdateGlobal(frameIndex, scene);

            m_frameData.UpdateUbo(frameIndex, scene.GetCamera(), scene.GetLight());
            m_frameData.UpdateSkybox(scene.GetSkybox());
            m_frameData.RefreshSet(frameIndex);
        }

        void RenderScene::UpdateGlobal(uint32_t frameIndex, const Scene::Scene& scene)
        {
            Gpu::PostProcess postProcess{};
            postProcess.m_exposure =
                glm::vec4(std::exp2(scene.GetPostProcess().GetEV()), 0.0f, 0.0f, 0.0f);
            m_global.m_postProcessSet.WriteData(frameIndex, postProcess);
        }

        void RenderScene::Recreate(VkExtent2D extent)
        {
            m_global.m_target = CreateTarget(extent);
            m_global.m_postProcessSet =
                m_resourceMgr.CreatePostProcessSet(m_global.m_target.m_resolveTexture);
        }

        VkDescriptorSetLayout RenderScene::GetEmptyLayout() const
        {
            return m_descriptorMgr.GetLayout(Resource::DescriptorLayoutType::Empty);
        }

        Resource::TargetResource RenderScene::CreateTarget(VkExtent2D extent) const
        {
            Resource::TargetDesc desc{};
            desc.m_extent = extent;
            desc.m_colorFormat = m_context.HdrFormat();
            desc.m_depthFormat = m_context.DepthFormat();
            desc.m_msaaSamples = m_context.SampleCount();

            // Linear + clamp + no mips: render target sampling.
            Resource::SamplerDesc samplerDesc{};
            samplerDesc.m_magFilter = VK_FILTER_LINEAR;
            samplerDesc.m_minFilter = VK_FILTER_LINEAR;
            samplerDesc.m_mipMode = Resource::SamplerDesc::MipMode::None;
            samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            desc.m_samplerDesc = samplerDesc;

            return m_resourceMgr.CreateTarget(desc);
        }

        ObjectState RenderScene::CreateObject() const
        {
            ObjectState object{};

            Resource::MaterialDesc matDesc{};
            matDesc.m_textureIds = object.m_lastTextureIds;
            object.m_materialSet = m_resourceMgr.GetOrCreatePerMaterialSet(matDesc);
            object.m_objectSet = m_resourceMgr.CreatePerObjectSet();

            return object;
        }

        void RenderScene::UpdateObject(ObjectState& object, uint32_t frameIndex, const Scene::Object& sceneObject)
        {
            // Mesh
            const Resource::ResourceId meshId = sceneObject.GetMesh().GetId();
            if (meshId != object.m_lastMeshId)
            {
                object.m_lastMeshId = meshId;
                object.m_mesh = m_resourceMgr.GetOrCreateMesh(meshId);
            }

            auto& mat = sceneObject.GetMaterial();

            // Material set
            bool anyTexUpdated = false;
            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                const Resource::ResourceId textureId = mat.GetTexture(static_cast<Resource::MaterialSlot>(i)).GetId();
                if (object.m_lastTextureIds[i] != textureId)
                {
                    object.m_lastTextureIds[i] = textureId;
                    anyTexUpdated = true;
                }
            }
            if (anyTexUpdated)
            {
                Resource::MaterialDesc desc{};
                desc.m_textureIds = object.m_lastTextureIds;
                object.m_materialSet = m_resourceMgr.GetOrCreatePerMaterialSet(desc);
            }

            // Object set
            Gpu::PerObject data{};
            data.m_transform.m_model = glm::mat4(1.0f);
            data.m_transform.m_normal = glm::mat4(1.0f);
            data.m_material.m_albedo = mat.GetAlbedo();
            data.m_material.m_pbrParams = glm::vec4(mat.GetMetallic(), mat.GetRoughness(), mat.GetAO(), 0.0f);
            data.m_material.m_emissive = glm::vec4(mat.GetEmissive(), 1.0f);
            object.m_objectSet.WriteData(frameIndex, data);
        }
    }
}
