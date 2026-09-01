#include "renderer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "rhi/context.h"
#include "rhi/frame_sync.h"
#include "rhi/swap_chain.h"
#include "rhi/utils.h"
#include "resource/descriptor_manager.h"
#include "resource/resource_manager.h"
#include "render/render_pipeline.h"
#include "render/render_scene.h"

#include <cassert>
#include <fstream>

namespace Kita::Pbrv
{
    namespace Render
    {
        Renderer::Renderer(Core::Window& window, const Resource::AssetManager& assetMgr)
        {
            m_context = std::make_unique<Rhi::Context>(window);
            m_swapChain = std::make_unique<Rhi::SwapChain>(window, *m_context);
            m_frameSync = std::make_unique<Rhi::FrameSync>(*m_context, *m_swapChain);

            m_descriptorMgr = std::make_unique<Resource::DescriptorManager>(*m_context);
            m_resourceMgr = std::make_unique<Resource::ResourceManager>(*m_context, assetMgr, *m_descriptorMgr);

            m_renderScene = std::make_unique<RenderScene>(*m_context, *m_swapChain, *m_descriptorMgr, *m_resourceMgr);
            m_pipeline = std::make_unique<RenderPipeline>(window, *m_context, *m_swapChain, *m_renderScene);
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

        void Renderer::DrawFrame()
        {
            auto frameInfo = m_frameSync->BeginFrame();

            // Deferred destruction advances every frame, recreate included:
            // recreating the render target also queues old textures
            m_resourceMgr->FlushGraveyard();

            if (frameInfo.m_swapChainRecreated)
            {
                // Recreate
                m_renderScene->Recreate();
                m_pipeline->RecreateResources();

                return;
            }

            // Update scene data
            m_renderScene->Update(frameInfo);

            // Draw
            m_pipeline->Draw(frameInfo);

            if (m_frameSync->EndFrame())
            {
                // Recreate
                m_renderScene->Recreate();
                m_pipeline->RecreateResources();
            }
        }
    }
}
