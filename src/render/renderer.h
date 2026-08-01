#pragma once

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class Window;
    class RenderContext;
    class SwapChain;
    class FrameSync;

    class RenderImage;

    class Renderer
    {
    public:
        Renderer(Window& window);
        ~Renderer();

        void DrawFrame();

    private:
        void CreatePipeline();
        void CreateDepthImage();
        void DestroyDepthImage();
        void RecreateDepthImage();
        VkShaderModule CreateShaderModule(const std::string& filePath) const;
        std::vector<char> ReadFile(const std::string& path) const;

    private:
        std::unique_ptr<RenderContext> m_context;
        std::unique_ptr<SwapChain> m_swapChain;
        std::unique_ptr<FrameSync> m_frameSync;

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };

        std::unique_ptr<RenderImage> m_depthImage;
        VkImageView m_depthImageView{ VK_NULL_HANDLE };
    };
}