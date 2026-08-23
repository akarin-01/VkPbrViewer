#include "renderer.h"

#include "render/render_context.h"
#include "render/swap_chain.h"
#include "render/frame_sync.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/render_scene.h"
#include "render/render_utils.h"
#include "render/render_pipeline.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <fstream>
#include <cassert>

namespace Kita::Pbrv
{
    namespace
    {
        std::unique_ptr<DescriptorAllocator> CreateDescriptorAllocator(const RenderContext& context)
        {
            /* Owner                |  Sets  |  UBO  |  Sampler  |  Storage  |
             * RenderFrameData      | frames |   1   |     0     |     0     |
             * RenderMaterialData   | frames |   0   | materials |     0     |
             * RenderSkyboxData     | frames |   0   |     1     |     0     |
             * RenderTargetData     |   1    |   0   |     1     |     0     |
             * RenderSkyboxData     |   1    |   0   |     1     |     1     |
             * RenderPostProcessData| frames |   1   |     0     |     0     |
             * RenderIblData        | frames |   0   |     1     |     0     |
             * RenderIblData        |   1    |   0   |     1     |     1     |
            */
            constexpr uint32_t kMaxSets = kMaxFramesInFlight * 5 + 3;
            constexpr uint32_t kMaxUboCount = kMaxFramesInFlight * 2;
            constexpr uint32_t kMaxSamplerCount = kMaxFramesInFlight * kMaterialTextureCount
                + kMaxFramesInFlight * 2 + 3;
            constexpr uint32_t kMaxStorageCount = 2;

            std::array<VkDescriptorPoolSize, 3> poolSizes
            {
                VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kMaxUboCount },
                VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxSamplerCount },
                VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kMaxStorageCount },
            };

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = kMaxSets;

            return std::make_unique<DescriptorAllocator>(context, poolInfo);
        }
    }

    Renderer::Renderer(Window& window)
    {
        m_context = std::make_unique<RenderContext>(window);
        m_resources = std::make_unique<RenderResources>(*m_context);
        m_swapChain = std::make_unique<SwapChain>(window, *m_context, *m_resources);
        m_frameSync = std::make_unique<FrameSync>(*m_context, *m_swapChain);

        m_descriptorAllocator = std::move(CreateDescriptorAllocator(*m_context));

        m_renderScene = std::make_unique<RenderScene>(*m_context, *m_resources, *m_swapChain, *m_descriptorAllocator);

        m_pipeline = std::make_unique<RenderPipeline>(window, *m_context, *m_resources, *m_swapChain, *m_descriptorAllocator, *m_renderScene);
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

    void Renderer::NewFrame() const
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
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
        m_pipeline->Draw(frameInfo);

        if (m_frameSync->EndFrame())
        {
            // Recreate
            m_pipeline->RecreateResources();
        }
    }
}
