#pragma once

#include "rhi/frame_info.h"

#include <memory>
#include <vector>

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
        struct TargetResource;
        struct TextureResource;
    }

    namespace Render
    {
        class RenderPassBase;
        class RenderScene;

        class RenderPipeline
        {
        public:
            RenderPipeline(const Core::Window& window,
                const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                RenderScene& scene);
            ~RenderPipeline();

            void RecreateResources();
            void Draw(const Rhi::FrameInfo& frameInfo) const;

        private:
            void CreateRenderPasses(const Core::Window& window,
                const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);

            void TransitionShadowMapToWriteLayout(VkCommandBuffer commandBuffer) const;
            void TransitionShadowMapToReadLayout(VkCommandBuffer commandBuffer) const;
            void TransitionTargetToWriteLayout(VkCommandBuffer commandBuffer) const;
            void TransitionTargetToReadLayout(VkCommandBuffer commandBuffer) const;
            void TransitionSwapchainToWriteLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
            void TransitionSwapchainToPresentLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;

        private:
            const Rhi::SwapChain& m_swapChain;
            Resource::TargetResource& m_target;
            Resource::TextureResource& m_shadowMap;

            std::unique_ptr<RenderPassBase> m_shadowPass;
            std::unique_ptr<RenderPassBase> m_litPass;
            std::unique_ptr<RenderPassBase> m_skyboxPass;
            std::unique_ptr<RenderPassBase> m_postProcessPass;
            std::unique_ptr<RenderPassBase> m_uiPass;
        };
    }
}
