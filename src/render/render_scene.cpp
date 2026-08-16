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
        m_materialData(context, resources, descriptorAllocator),
        m_meshData(resources),
        m_skyboxData(context, resources, descriptorAllocator)
    {
    }

    RenderScene::~RenderScene() = default;

    void RenderScene::Update(const Scene& scene, const FrameInfo& frameInfo)
    {
        auto& frameIndex = frameInfo.m_frameIndex;

        m_frameData.Update(frameIndex, scene.GetCamera(), scene.GetLight());
        m_materialData.Update(frameIndex, scene.GetMaterial());
        m_meshData.Update(scene.GetMesh());
        m_skyboxData.Update(frameIndex, scene.GetSkybox());
    }
}
