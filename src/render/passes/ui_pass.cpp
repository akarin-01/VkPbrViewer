#include "ui_pass.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "core/window.h"
#include "render/render_context.h"
#include "render/swap_chain.h"
#include "render/rendering_scope.h"

#include <array>
#include <stdexcept>

namespace Kita::Pbrv
{
    UIPass::UIPass(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const Window& window)
        : RenderPassBase(context, resources, swapChain)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplVulkan_InitInfo info{};
        info.ApiVersion = VK_API_VERSION_1_3;
        info.Instance = m_context.Instance();
        info.PhysicalDevice = m_context.PhysicalDevice();
        info.Device = m_context.Device();
        info.QueueFamily = m_context.GraphicsFamily();
        info.Queue = m_context.GraphicsQueue();
        info.DescriptorPool = VK_NULL_HANDLE;
        info.DescriptorPoolSize = 8;
        info.MinImageCount = static_cast<uint32_t>(m_swapChain.ImageCount());
        info.ImageCount = static_cast<uint32_t>(m_swapChain.ImageCount());
        info.UseDynamicRendering = true;

        std::array<VkFormat, 1> colorFormats
        {
            m_swapChain.Format()
        };
        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
        info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

        ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(window.GetNativeHandle()), true);

        if (!ImGui_ImplVulkan_Init(&info))
        {
            throw std::runtime_error("Failed to initialize ImGui Vulkan backend!");
        }
    }

    UIPass::~UIPass()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void UIPass::RecreateResources()
    {
        /* Swapchain recreation may change the image count; format is assumed unchanged.
         * In dynamic rendering mode the Vulkan backend owns no framebuffers,
         * so only the minimum image count needs to be refreshed.
         */
        ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(m_swapChain.ImageCount()));
    }

    void UIPass::Draw(const FrameInfo& frameInfo) const
    {
        ImGui::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        if (!drawData || drawData->CmdListsCount == 0)
        {
            return;
        }

        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& imageIndex = frameInfo.m_imageIndex;

        // Begin rendering
        {
            RenderingAttachmentDesc colorDesc{};
            colorDesc.m_imageView = m_swapChain.ImageView(imageIndex);
            colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            RenderingScope scope(commandBuffer, m_swapChain.Extent(), { colorDesc });
            ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        }
        // End rendering
    }
}
