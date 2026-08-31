#include "render_scene.h"

#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_manager.h"
#include "resource/gpu_layouts.h"
#include "resource/resource_manager.h"
#include "scene/camera.h"
#include "scene/light.h"
#include "scene/scene.h"

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Render
    {
        RenderScene::RenderScene(const Rhi::Context& context,
            const Rhi::SwapChain& swapChain,
            Resource::DescriptorManager& descriptorMgr,
            Resource::ResourceManager& resourceMgr)
            : m_context(context),
            m_swapChain(swapChain),
            m_resourceMgr(resourceMgr),
            m_descriptorMgr(descriptorMgr)
        {
            Recreate();
            m_global.m_frameSet = m_resourceMgr.CreatePerFrameSet(m_global.m_lastEquirectId);
            m_object = CreateObject();
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;

            UpdateFrameSet(frameIndex, scene);
            UpdatePostProcessSet(frameIndex, scene);
            UpdateObject(m_object, frameIndex, scene.GetObject());
        }

        void RenderScene::UpdateFrameSet(uint32_t frameIndex, const Scene::Scene& scene)
        {
            const Resource::ResourceId equirectId = scene.GetSkybox().GetSkybox().GetId();
            if (m_global.m_lastEquirectId != equirectId)
            {
                m_global.m_lastEquirectId = equirectId;
                m_global.m_frameSet = m_resourceMgr.CreatePerFrameSet(equirectId);
            }

            Gpu::PerFrame perFrame{};
            glm::mat4 view = scene.GetCamera().GetViewMatrix();
            glm::mat4 proj = scene.GetCamera().GetProjectMatrix(m_swapChain.Aspect());
            perFrame.m_camera.m_viewProj = proj * view;
            perFrame.m_camera.m_skyboxViewProj = proj * glm::mat4(glm::mat3(view));
            perFrame.m_camera.m_position = glm::vec4(scene.GetCamera().GetPosition(), 1.0f);
            perFrame.m_light.m_position = glm::vec4(scene.GetLight().GetPosition(), 0.0f);
            perFrame.m_light.m_colorIntensity =
                glm::vec4(scene.GetLight().GetColor(), scene.GetLight().GetIntensity());
            m_global.m_frameSet.WriteData(frameIndex, perFrame);
        }

        void RenderScene::UpdatePostProcessSet(uint32_t frameIndex, const Scene::Scene& scene)
        {
            Gpu::PostProcess postProcess{};
            postProcess.m_exposure =
                glm::vec4(std::exp2(scene.GetPostProcess().GetEV()), 0.0f, 0.0f, 0.0f);
            m_global.m_postProcessSet.WriteData(frameIndex, postProcess);
        }

        void RenderScene::Recreate()
        {
            Resource::TargetDesc desc{};
            desc.m_extent = m_swapChain.Extent();
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

            m_global.m_target = m_resourceMgr.CreateTarget(desc);

            m_global.m_postProcessSet =
                m_resourceMgr.CreatePostProcessSet(m_global.m_target.m_resolveTexture);
        }

        VkDescriptorSetLayout RenderScene::GetEmptyLayout() const
        {
            return m_descriptorMgr.GetLayout(Resource::DescriptorLayoutType::Empty);
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

        void RenderScene::UpdateObject(ObjectState& object, uint32_t frameIndex, const Scene::Object& sceneObject) const
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
