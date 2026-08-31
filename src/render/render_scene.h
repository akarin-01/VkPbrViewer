#pragma once

#include "rhi/frame_info.h"
#include "render/scene_state.h"
#include "render/data/render_frame_data.h"

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
        class Resources;
        class DescriptorManager;
        class ResourceManager;
    }
    namespace Scene
    {
        class Scene;
        class Object;
    }

    namespace Render
    {
        /// Owns the GPU-side scene state and syncs the CPU scene data each frame.
        /// Passes get their dependencies injected via GetXxx.
        class RenderScene
        {
        public:
            RenderScene(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                Resource::DescriptorManager& descriptorMgr,
                Resource::ResourceManager& resourceMgr);
            ~RenderScene();

            void Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo);
            void Recreate(VkExtent2D extent);

            const RenderFrameData& GetFrameData() const { return m_frameData; }
            const GlobalState& GetGlobal() const { return m_global; }
            GlobalState& GetGlobal() { return m_global; }
            const ObjectState& GetObject() const { return m_object; }

            VkDescriptorSetLayout GetEmptyLayout() const;

        private:
            Resource::TargetResource CreateTarget(VkExtent2D extent) const;
            ObjectState CreateObject() const;
            void UpdateGlobal(uint32_t frameIndex, const Scene::Scene& scene);
            void UpdateObject(ObjectState& object, uint32_t frameIndex, const Scene::Object& sceneObject);

        private:
            const Rhi::Context& m_context;
            Resource::ResourceManager& m_resourceMgr;
            Resource::DescriptorManager& m_descriptorMgr;

            GlobalState m_global{};
            ObjectState m_object{};

            RenderFrameData m_frameData;
        };
    }
}
