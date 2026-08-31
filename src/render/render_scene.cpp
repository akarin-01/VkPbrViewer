#include "render_scene.h"

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
            m_target(context, resources, swapChain.Extent()),
            m_frameData(context, resources, swapChain, descriptorMgr),
            m_postProcessData(context, resources, descriptorMgr, m_target.GetResolveTexture())
        {
            m_objectState = CreateObjectState();
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;

            UpdateObject(m_objectState, frameIndex, scene.GetObject());

            m_frameData.UpdateUbo(frameIndex, scene.GetCamera(), scene.GetLight());
            m_frameData.UpdateSkybox(scene.GetSkybox());
            m_frameData.RefreshSet(frameIndex);

            m_postProcessData.UpdateUbo(frameIndex, scene.GetPostProcess());
            m_postProcessData.RefreshSet(frameIndex);
        }

        void RenderScene::Recreate(VkExtent2D extent)
        {
            m_target.Recreate(extent);
            m_postProcessData.UpdateTarget(m_target.GetResolveTexture());
        }

        VkDescriptorSetLayout RenderScene::GetEmptyLayout() const
        {
            return m_descriptorMgr.GetLayout(Resource::DescriptorLayoutType::Empty);
        }

        ObjectState RenderScene::CreateObjectState() const
        {
            ObjectState object{};

            Resource::MaterialDesc matDesc{};
            matDesc.m_textureIds = object.m_lastTextureIds;
            object.m_materialSet = m_resourceMgr.GetOrCreatePerMaterialSet(matDesc);
            object.m_objectSet = m_resourceMgr.CreatePerObjectSet();

            return object;
        }

        void RenderScene::UpdateObject(ObjectState& state, uint32_t frameIndex, const Scene::Object& object)
        {
            // Mesh
            const Resource::ResourceId meshId = object.GetMesh().GetId();
            if (meshId != state.m_lastMeshId)
            {
                state.m_lastMeshId = meshId;
                state.m_mesh = m_resourceMgr.GetOrCreateMesh(meshId);
            }

            auto& mat = object.GetMaterial();

            // Material set
            bool anyTexUpdated = false;
            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                const Resource::ResourceId textureId = mat.GetTexture(static_cast<Resource::MaterialSlot>(i)).GetId();
                if (state.m_lastTextureIds[i] != textureId)
                {
                    state.m_lastTextureIds[i] = textureId;
                    anyTexUpdated = true;
                }
            }
            if (anyTexUpdated)
            {
                Resource::MaterialDesc desc{};
                desc.m_textureIds = state.m_lastTextureIds;
                state.m_materialSet = m_resourceMgr.GetOrCreatePerMaterialSet(desc);
            }

            // Object set
            Gpu::PerObject data{};
            data.m_transform.m_model = glm::mat4(1.0f);
            data.m_transform.m_normal = glm::mat4(1.0f);
            data.m_material.m_albedo = mat.GetAlbedo();
            data.m_material.m_pbrParams = glm::vec4(mat.GetMetallic(), mat.GetRoughness(), mat.GetAO(), 0.0f);
            data.m_material.m_emissive = glm::vec4(mat.GetEmissive(), 1.0f);
            state.WriteData(frameIndex, data);
        }
    }
}
