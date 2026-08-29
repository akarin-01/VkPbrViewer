#include "render_scene.h"
#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_manager.h"
#include "resource/resource_manager.h"
#include "resource/resources.h"
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
            : m_resourceMgr(resourceMgr),
            m_descriptorMgr(descriptorMgr),
            m_target(context, resources, swapChain.Extent()),
            m_frameData(context, resources, swapChain, descriptorMgr),
            m_materialData(context, resources, descriptorMgr),
            m_objectData(context, resources, descriptorMgr),
            m_postProcessData(context, resources, descriptorMgr, m_target.GetResolveTexture())
        {
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;

            UpdateMesh(scene.GetObject());

            m_frameData.UpdateUbo(frameIndex, scene.GetCamera(), scene.GetLight());
            m_frameData.UpdateSkybox(scene.GetSkybox());
            m_frameData.RefreshSet(frameIndex);

            m_materialData.UpdateTextures(scene.GetObject().GetMaterial());
            m_materialData.RefreshSet(frameIndex);

            m_objectData.UpdateUbo(frameIndex, scene.GetObject());

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

        void RenderScene::UpdateMesh(const Scene::Object& object)
        {
            const Resource::ResourceId meshId = object.GetMesh().GetId();
            if (meshId != m_meshData.m_lastMeshId)
            {
                m_meshData.m_lastMeshId = meshId;
                m_meshData.m_mesh = m_resourceMgr.GetOrCreateMeshResource(meshId);
            }
        }
    }
}
