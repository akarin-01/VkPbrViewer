#include "render_scene.h"

#include "render/render_context.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/swap_chain.h"
#include "scene/scene.h"

namespace Kita::Pbrv
{
    RenderScene::RenderScene(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator)
        : m_frameData(context, resources, swapChain, descriptorAllocator),
        m_materialCache(context, resources, descriptorAllocator),
        m_meshCache(resources),
        m_skyboxEnvironment(context, resources, descriptorAllocator)
    {
    }

    RenderScene::~RenderScene() = default;

    void RenderScene::Update(const Scene& scene, const FrameInfo& frameInfo)
    {
        auto& frameIndex = frameInfo.m_frameIndex;

        m_frameData.Update(frameIndex, scene.GetCamera(), scene.GetLight());
        m_materialCache.Update(frameIndex, scene.GetMaterial());
        m_meshCache.Update(scene.GetMesh());
        m_skyboxEnvironment.Update(frameIndex, scene.GetSkybox());
    }
}
