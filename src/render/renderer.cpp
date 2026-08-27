#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "renderer.h"
#include "rhi/context.h"
#include "rhi/frame_sync.h"
#include "rhi/swap_chain.h"
#include "rhi/utils.h"
#include "resource/descriptor_manager.h"
#include "resource/resources.h"
#include "render/render_pipeline.h"
#include "render/render_scene.h"

#include <fstream>
#include <cassert>

namespace Kita::Pbrv
{
    namespace Render
    {
        Renderer::Renderer(Core::Window& window)
        {
            m_context = std::make_unique<Rhi::RenderContext>(window);
            m_resources = std::make_unique<Resource::RenderResources>(*m_context);
            m_swapChain = std::make_unique<Rhi::SwapChain>(window, *m_context, *m_resources);
            m_frameSync = std::make_unique<Rhi::FrameSync>(*m_context, *m_swapChain);

            m_descriptorMgr = std::make_unique<Resource::DescriptorManager>(*m_context);

            m_renderScene = std::make_unique<RenderScene>(*m_context, *m_resources, *m_swapChain, *m_descriptorMgr);
            m_pipeline = std::make_unique<RenderPipeline>(window, *m_context, *m_resources, *m_swapChain, *m_descriptorMgr, *m_renderScene);
        }

        Renderer::~Renderer()
        {
            vkDeviceWaitIdle(m_context->Device());
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
