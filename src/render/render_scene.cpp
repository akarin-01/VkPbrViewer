#include "render_scene.h"
#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_allocator.h"
#include "resource/resources.h"
#include "scene/scene.h"

namespace Kita::Pbrv
{
    namespace Render
    {
        RenderScene::RenderScene(const Rhi::RenderContext& context,
            Resource::RenderResources& resources,
            const Rhi::SwapChain& swapChain,
            const Resource::DescriptorAllocator& descriptorAllocator)
            : m_frameData(context, resources, swapChain, descriptorAllocator),
            m_materialData(context, resources, descriptorAllocator),
            m_meshData(resources),
            m_skyboxData(context, resources, descriptorAllocator),
            m_postProcessData(context, resources, descriptorAllocator),
            m_iblData(context, resources, descriptorAllocator)
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
