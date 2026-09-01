#pragma once

#include "render/render_pass_base.h"
#include "resource/resource_id.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class GraphicsPipeline;
    }
    namespace Resource
    {
        struct PerFrameSet;
        struct TargetResource;
    }

    namespace Render
    {
        struct ObjectState;
        class RenderScene;

        class LitPass : public RenderPassBase
        {
        public:
            LitPass(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            ~LitPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts);

        private:
            const Resource::TargetResource& m_target;
            const Resource::PerFrameSet& m_frameSet;
            const std::unordered_map<Resource::ResourceId, ObjectState>& m_objectMap;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
