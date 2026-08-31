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
        class DescriptorManager;
        class ResourceManager;
    }
    namespace Scene
    {
        class Object;
        class Scene;
    }

    namespace Render
    {
        /// Owns the GPU-side scene state and syncs the CPU scene data each frame.
        /// Passes get their dependencies injected via GetXxx.
        class RenderScene
        {
        public:
            RenderScene(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                Resource::DescriptorManager& descriptorMgr,
                Resource::ResourceManager& resourceMgr);
            ~RenderScene();

            void Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo);
            void Recreate();

            const GlobalState& GetGlobal() const { return m_global; }
            GlobalState& GetGlobal() { return m_global; }
            const ObjectState& GetObject() const { return m_object; }

            VkDescriptorSetLayout GetEmptyLayout() const;

        private:
            ObjectState CreateObject() const;
            void UpdateFrameSet(uint32_t frameIndex, const Scene::Scene& scene);
            void UpdatePostProcessSet(uint32_t frameIndex, const Scene::Scene& scene);
            void UpdateObject(ObjectState& object, uint32_t frameIndex, const Scene::Object& sceneObject) const;

        private:
            const Rhi::Context& m_context;
            const Rhi::SwapChain& m_swapChain;
            Resource::ResourceManager& m_resourceMgr;
            Resource::DescriptorManager& m_descriptorMgr;

            GlobalState m_global{};
            ObjectState m_object{};
        };
    }
}
