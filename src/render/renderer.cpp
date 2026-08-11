#include "renderer.h"

#include "render/render_context.h"
#include "render/swap_chain.h"
#include "render/frame_sync.h"
#include "render/render_resources.h"
#include "render/render_scene.h"
#include "render/render_utils.h"
#include "render/render_pass.h"

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
        m_renderScene = std::make_unique<RenderScene>(*m_resources, *m_swapChain);

        m_pass = std::make_unique<RenderPass>(*m_context, *m_resources, *m_swapChain);

        RenderList list = m_renderScene->GetRenderList();
        m_pass->Initialize(list);
    }

    Renderer::~Renderer()
    {
        vkDeviceWaitIdle(m_context->Device());

        m_pass.reset();

        m_renderScene.reset();
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
            m_pass->RecreateResources();

            return;
        }

        // Update scene data
        m_renderScene->Update(scene, frameInfo);

        // Draw
        RenderList list = m_renderScene->GetRenderList();
        m_pass->Draw(list, frameInfo);

        if (m_frameSync->EndFrame())
        {
            // Recreate
            m_pass->RecreateResources();
        }
    }
}