#include "renderer.h"

#include "render/render_context.h"
#include "render/swap_chain.h"
#include "render/frame_sync.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/render_scene.h"
#include "render/render_utils.h"
#include "render/render_pipeline.h"

#include <fstream>
#include <cassert>

namespace Kita::Pbrv
{
    Renderer::Renderer(Window& window)
    {
        m_context = std::make_unique<RenderContext>(window);
        m_resources = std::make_unique<RenderResources>(*m_context);
        m_swapChain = std::make_unique<SwapChain>(window, *m_context, *m_resources);
        m_frameSync = std::make_unique<FrameSync>(*m_context, *m_swapChain);

        m_descriptorAllocator = std::move(CreateDescriptorAllocator(*m_context));

        m_renderScene = std::make_unique<RenderScene>(*m_context, *m_resources, *m_swapChain, *m_descriptorAllocator);

        auto& list = m_renderScene->GetRenderList();
        m_pipeline = std::make_unique<RenderPipeline>(*m_context, *m_resources, *m_swapChain, *m_descriptorAllocator, list);
    }

    Renderer::~Renderer()
    {
        vkDeviceWaitIdle(m_context->Device());

        m_pipeline.reset();
        m_renderScene.reset();
        m_descriptorAllocator.reset();
        m_frameSync.reset();
        m_swapChain.reset();
        m_resources.reset();
        m_context.reset();
    }

    void Renderer::DrawFrame(const Scene& scene)
    {
        auto frameInfo = m_frameSync->BeginFrame();

        if (frameInfo.m_swapChainRecreated)
        {
            // Recreate
            m_pipeline->RecreateResources();

            return;
        }

        m_resources->FlushDeferred(frameInfo.m_frameIndex);

        // Update scene data
        m_renderScene->Update(scene, frameInfo);

        // Draw
        auto& list = m_renderScene->GetRenderList();
        m_pipeline->Draw(list, frameInfo);

        if (m_frameSync->EndFrame())
        {
            // Recreate
            m_pipeline->RecreateResources();
        }
    }

    std::unique_ptr<DescriptorAllocator> Renderer::CreateDescriptorAllocator(const RenderContext& context)
    {
        std::vector<VkDescriptorPoolSize> poolSizes(2);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = kMaxFramesInFlight * 2;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = kMaxFramesInFlight * kMaterialTextureCount + kMaxFramesInFlight + 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = kMaxFramesInFlight * 4 + 1;

        return std::make_unique<DescriptorAllocator>(context, poolInfo);
    }
}