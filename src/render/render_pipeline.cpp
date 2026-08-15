#include "render_pipeline.h"

#include "render/render_pass_base.h"
#include "render/passes/lit_pass.h"

namespace Kita::Pbrv
{
    RenderPipeline::RenderPipeline(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator,
        const RenderList& list)
    {
        m_passes.push_back(std::make_unique<LitPass>(context, resources, swapChain, descriptorAllocator, list));
    }

    RenderPipeline::~RenderPipeline() = default;

    void RenderPipeline::RecreateResources()
    {
        for (auto& pass : m_passes)
        {
            pass->RecreateResources();
        }
    }

    void RenderPipeline::Draw(const RenderList& list, const FrameInfo& frameInfo) const
    {
        for (auto& pass : m_passes)
        {
            pass->Draw(list, frameInfo);
        }
    }
}