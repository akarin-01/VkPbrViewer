#pragma once

#include "rhi/frame_info.h"
#include "render/scene_state.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
        class SwapChain;
    }
    namespace Resource
    {
        class ResourceManager;
    }

    namespace Render
    {
        class SceneProxy;

        /// Owns the GPU-side scene state; consumes the SceneProxy each frame.
        /// Passes get their dependencies injected via GetXxx
        class RenderScene
        {
        public:
            RenderScene(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                Resource::ResourceManager& resourceMgr);
            ~RenderScene();

            void Update(const Rhi::FrameInfo& frameInfo);
            void Recreate();

            const GlobalState& GetGlobal() const { return m_global; }
            GlobalState& GetGlobal() { return m_global; }
            const ObjectState& GetObject() const { return m_object; }

            VkDescriptorSetLayout GetEmptyLayout() const;

        private:
            ObjectState CreateObject() const;
            void UpdateFrameSet(uint32_t frameIndex, const SceneProxy& proxy);
            void UpdatePostProcessSet(uint32_t frameIndex, const SceneProxy& proxy);
            void UpdateObject(uint32_t frameIndex, const SceneProxy& proxy);

        private:
            const Rhi::Context& m_context;
            const Rhi::SwapChain& m_swapChain;
            Resource::ResourceManager& m_resourceMgr;

            GlobalState m_global{};
            ObjectState m_object{};
        };
    }
}
