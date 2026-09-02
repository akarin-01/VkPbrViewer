#pragma once
#include "render/render_pass_base.h"
#include "resource/resource_id.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class GraphicsPipeline;
    }
    namespace Resource
    {
        struct PerFrameSet;
        struct TextureResource;
    }

    namespace Render
    {
        class RenderScene;
        struct ObjectState;

        class ShadowPass : public RenderPassBase
        {
        public:
            ShadowPass(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            ~ShadowPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts);

        private:
            const Resource::PerFrameSet& m_frameSet;
            const Resource::TextureResource& m_shadowMap;
            const std::unordered_map<Resource::ResourceId, ObjectState>& m_objectMap;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
