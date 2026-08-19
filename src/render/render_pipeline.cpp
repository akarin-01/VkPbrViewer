#include "render_pipeline.h"

#include "render/swap_chain.h"
#include "render/render_scene.h"

#include "render/passes/lit_pass.h"
#include "render/passes/skybox_pass.h"
#include "render/passes/post_process_pass.h"

#include <cassert>

namespace Kita::Pbrv
{
    RenderPipeline::RenderPipeline(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator,
        const RenderScene& scene)
        : m_swapChain(swapChain),
        m_targetData(context, resources, descriptorAllocator, m_swapChain.Extent())
    {
        CreateRenderPasses(context, resources, swapChain, scene);
    }

    RenderPipeline::~RenderPipeline()
    {
        DestroyRenderPasses();
    }

    void RenderPipeline::RecreateResources()
    {
        m_targetData.Recreate(m_swapChain.Extent());

        for (auto& pass : m_passes)
        {
            pass->RecreateResources();
        }
    }

    void RenderPipeline::Draw(const FrameInfo& frameInfo) const
    {
        for (auto& pass : m_passes)
        {
            pass->Draw(frameInfo);
        }
    }

    void RenderPipeline::CreateRenderPasses(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const RenderScene& scene)
    {
        m_passes.push_back(std::make_unique<LitPass>(
            context, resources, swapChain,
            m_targetData, scene.GetRenderFrameData(), scene.GetRenderMaterialData(), scene.GetRenderMeshData()));
        m_passes.push_back(std::make_unique<SkyboxPass>(
            context, resources, swapChain,
            m_targetData, scene.GetRenderFrameData(), scene.GetRenderSkyboxData()));
        m_passes.push_back(std::make_unique<PostProcessPass>(
            context, resources, swapChain,
            m_targetData));
    }

    void RenderPipeline::DestroyRenderPasses()
    {
        m_passes.clear();
    }
}