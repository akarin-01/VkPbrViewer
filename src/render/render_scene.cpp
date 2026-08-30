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
            m_materialData(context, resources, descriptorMgr),
            m_postProcessData(context, resources, descriptorMgr, m_target.GetResolveTexture())
        {
            m_objectState = CreateObjectState();
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;

            UpdateObjectState(frameIndex, scene.GetObject());

            m_frameData.UpdateUbo(frameIndex, scene.GetCamera(), scene.GetLight());
            m_frameData.UpdateSkybox(scene.GetSkybox());
            m_frameData.RefreshSet(frameIndex);

            m_materialData.UpdateTextures(scene.GetObject().GetMaterial());
            m_materialData.RefreshSet(frameIndex);

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
            object.m_set = m_resourceMgr.CreatePerObjectSet();

            return object;
        }

        void RenderScene::UpdateObjectState(uint32_t frameIndex, const Scene::Object& object)
        {
            const Resource::ResourceId meshId = object.GetMesh().GetId();
            if (meshId != m_objectState.m_lastMeshId)
            {
                m_objectState.m_lastMeshId = meshId;
                m_objectState.m_mesh = m_resourceMgr.GetOrCreateMesh(meshId);
            }

            auto& mat = object.GetMaterial();
            Gpu::PerObject data{};
            data.m_transform.m_model = glm::mat4(1.0f);
            data.m_transform.m_normal = glm::mat4(1.0f);
            data.m_material.m_albedo = mat.GetAlbedo();
            data.m_material.m_pbrParams = glm::vec4(mat.GetMetallic(), mat.GetRoughness(), mat.GetAO(), 0.0f);
            data.m_material.m_emissive = glm::vec4(mat.GetEmissive(), 1.0f);
            m_objectState.WriteData(frameIndex, data);
        }
    }
}
