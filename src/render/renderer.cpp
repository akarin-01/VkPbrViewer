#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "renderer.h"
#include "rhi/constants.h"
#include "rhi/context.h"
#include "rhi/frame_sync.h"
#include "rhi/swap_chain.h"
#include "rhi/utils.h"
#include "resource/descriptor_allocator.h"
#include "resource/resources.h"
#include "render/render_pipeline.h"
#include "render/render_scene.h"

#include <fstream>
#include <cassert>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            std::unique_ptr<Resource::DescriptorAllocator> CreateDescriptorAllocator(const Rhi::RenderContext& context)
            {
                /* Owner                           |  Sets  |  UBO  |  Sampler  |  Storage  |
                 * RenderFrameData                 | frames |   1   |     0     |     0     |
                 * RenderMaterialData              | frames |   0   | materials |     0     |
                 * RenderSkyboxData                | frames |   0   |     1     |     0     |
                 * RenderPostProcessData           | frames |   1   |     0     |     0     |
                 * RenderIblData                   | frames |   0   |     2     |     0     |
                 * RenderTargetData                |   1    |   0   |     1     |     0     |
                 * Scene::Skybox conversion               |   1    |   0   |     1     |     1     |
                 * Ibl conv: brdf                  |   1    |   0   |     0     |     1     |
                 * Ibl conv: irradiance            |   1    |   0   |     1     |     1     |
                 * Ibl conv: prefilter             |   1    |   0   |     1     |     1     |
                 * Ibl brdf lut set                |   1    |   0   |     1     |     0     |
                */
                // Static: 6 sets / 5 samplers / 4 storages (target + 4 conversions + brdf lut)
                constexpr uint32_t kMaxSets = Rhi::kMaxFramesInFlight * 5 + 6;
                constexpr uint32_t kMaxUboCount = Rhi::kMaxFramesInFlight * 2;
                constexpr uint32_t kMaxSamplerCount = Rhi::kMaxFramesInFlight * (Resource::kMaterialTextureCount + 3) + 5;
                constexpr uint32_t kMaxStorageCount = 4;

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

                return std::make_unique<Resource::DescriptorAllocator>(context, poolInfo);
            }
        }

        Renderer::Renderer(Core::Window& window)
        {
            m_context = std::make_unique<Rhi::RenderContext>(window);
            m_resources = std::make_unique<Resource::RenderResources>(*m_context);
            m_swapChain = std::make_unique<Rhi::SwapChain>(window, *m_context, *m_resources);
            m_frameSync = std::make_unique<Rhi::FrameSync>(*m_context, *m_swapChain);

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

        void Renderer::DrawFrame(const Scene::Scene& scene)
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
}
