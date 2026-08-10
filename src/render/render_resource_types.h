#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    struct RenderBuffer
    {
        VkBuffer m_buffer{ VK_NULL_HANDLE };
        VkDeviceMemory m_memory{ VK_NULL_HANDLE };
        void* m_mapped{ nullptr };
    };

    struct RenderImage
    {
        VkImage m_image{ VK_NULL_HANDLE };
        VkDeviceMemory m_memory{ VK_NULL_HANDLE };
    };

    using RenderBufferHandle = uint64_t;
    using RenderImageHandle = uint64_t;

    struct FrameInfo
    {
        bool m_swapChainRecreated{ false };
        VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
        uint32_t m_frameIndex{ 0 };
        uint32_t m_imageIndex{ 0 };
    };

    struct RenderCamera
    {
        std::vector<RenderBufferHandle> m_viewProjUboHandles;
    };

    struct RenderMesh
    {
        RenderBufferHandle m_vertexBufferHandle{ 0 };
        RenderBufferHandle m_indexBufferHandle{ 0 };
        uint32_t m_indexCount{ 0 };
    };

    struct RenderList
    {
        RenderCamera m_camera;
        RenderMesh m_mesh;
    };

    struct FrameUbo
    {
        alignas(16) glm::mat4 viewProj;
    };
    static_assert(sizeof(FrameUbo) == 64, "std140 mismatch");
}