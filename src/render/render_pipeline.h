#pragma once

#include "rhi/frame_info.h"
#include "resource/types.h"

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
        struct TargetResource;
    }

    namespace Render
    {
        class RenderScene;
        class RenderPassBase;

        class RenderPipeline
        {
        public:
            RenderPipeline(const Core::Window& window,
                const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                RenderScene& scene);
            ~RenderPipeline();

            void RecreateResources();
            void Draw(const Rhi::FrameInfo& frameInfo) const;

        private:
            void CreateRenderPasses(const Core::Window& window,
                const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);

            void TransitionTargetToWriteLayout(VkCommandBuffer commandBuffer) const;
            void TransitionTargetToReadLayout(VkCommandBuffer commandBuffer) const;
            void TransitionSwapchainToWriteLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
            void TransitionSwapchainToPresentLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;

        private:
            const Rhi::SwapChain& m_swapChain;
            Resource::TargetResource& m_target;

            std::unique_ptr<RenderPassBase> m_litPass;
            std::unique_ptr<RenderPassBase> m_skyboxPass;
            std::unique_ptr<RenderPassBase> m_postProcessPass;
            std::unique_ptr<RenderPassBase> m_uiPass;
        };
    }
}
