#include "render_scene.h"
#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_manager.h"
#include "resource/resources.h"
#include "scene/scene.h"

namespace Kita::Pbrv
{
    namespace Render
    {
        RenderScene::RenderScene(const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            Resource::DescriptorManager& descriptorMgr)
            : m_frameData(context, resources, swapChain, descriptorMgr),
            m_materialData(context, resources, descriptorMgr),
            m_meshData(resources),
            m_skyboxData(context, resources, descriptorMgr),
            m_postProcessData(context, resources, descriptorMgr),
            m_iblData(context, resources, descriptorMgr)
        {
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;

            m_frameData.Update(frameIndex, scene.GetCamera(), scene.GetLight());
            m_meshData.Update(scene.GetObject().GetMesh());
            m_materialData.Update(frameIndex, scene.GetObject().GetMaterial());
            m_skyboxData.Update(frameIndex, scene.GetSkybox());
            m_postProcessData.Update(frameIndex, scene.GetPostProcess());
            m_iblData.Update(frameIndex, m_skyboxData.GetCubemap());
        }
    }
}
