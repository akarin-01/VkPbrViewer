#pragma once

#include "rhi/frame_info.h"
#include "render/scene_state.h"
#include "render/render_target.h"
#include "render/data/render_frame_data.h"
#include "render/data/render_material_data.h"
#include "render/data/render_post_process_data.h"

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
            const RenderMaterialData& GetMaterialData() const { return m_materialData; }
            const ObjectState& GetObjectState() const { return m_objectState; }
            const RenderPostProcessData& GetPostProcessData() const { return m_postProcessData; }

            const RenderTarget& GetTarget() const { return m_target; }
            RenderTarget& GetTarget() { return m_target; }

            VkDescriptorSetLayout GetEmptyLayout() const;

        private:
            ObjectState CreateObjectState() const;
            void UpdateObjectState(uint32_t frameIndex, const Scene::Object& object);

        private:
            const Rhi::Context& m_context;
            Resource::ResourceManager& m_resourceMgr;
            Resource::DescriptorManager& m_descriptorMgr;

            RenderTarget m_target;

            RenderFrameData m_frameData;
            RenderMaterialData m_materialData;
            ObjectState m_objectState{};

            RenderPostProcessData m_postProcessData;
        };
    }
}
