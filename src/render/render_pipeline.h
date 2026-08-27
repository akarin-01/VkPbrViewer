#pragma once

#include "rhi/frame_info.h"
#include "resource/types.h"
#include "render/data/render_target_data.h"

#include <vector>
#include <memory>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Window;
    }
    namespace Rhi
    {
        class Context;
        class SwapChain;
    }
    namespace Resource
    {
        class Resources;
        class DescriptorManager;
    }

    namespace Render
    {
        class RenderScene;
        class RenderPassBase;
        class RenderTargetData;
        class RenderPipeline
        {
        public:
            RenderPipeline(const Core::Window& window,
                const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                Resource::DescriptorManager& descriptorMgr,
                const RenderScene& scene);
            ~RenderPipeline();

            void RecreateResources();
            void Draw(const Rhi::FrameInfo& frameInfo) const;

        private:
            void CreateRenderPasses(const Core::Window& window,
                const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            void DestroyRenderPasses();

        private:
            const Rhi::SwapChain& m_swapChain;

            std::vector<std::unique_ptr<RenderPassBase>> m_passes;
            RenderTargetData m_targetData;
        };
    }
}
