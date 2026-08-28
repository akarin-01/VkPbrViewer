#include "render_scene.h"
#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_manager.h"
#include "resource/resource_manager.h"
#include "resource/resources.h"
#include "scene/scene.h"

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
            m_frameData(context, resources, swapChain, descriptorMgr),
            m_materialData(context, resources, descriptorMgr),
            m_skyboxData(context, resources, descriptorMgr),
            m_postProcessData(context, resources, descriptorMgr),
            m_iblData(context, resources, descriptorMgr)
        {
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;

            UpdateObject(scene.GetObject());

            m_frameData.Update(frameIndex, scene.GetCamera(), scene.GetLight());
            m_materialData.Update(frameIndex, scene.GetObject().GetMaterial());
            m_skyboxData.Update(frameIndex, scene.GetSkybox());
            m_postProcessData.Update(frameIndex, scene.GetPostProcess());
            m_iblData.Update(frameIndex, m_skyboxData.GetCubemap());
        }

        void RenderScene::UpdateObject(const Scene::Object& object)
        {
            const Resource::ResourceId meshId = object.GetMesh().GetId();
            if (meshId != m_object.m_lastMeshId)
            {
                m_object.m_lastMeshId = meshId;
                m_object.m_mesh = m_resourceMgr.GetOrCreateMeshResource(meshId);
            }
        }
    }
}
