#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

#define STD140_ASSERT(T, SIZE)\
    static_assert(sizeof(T) == SIZE, #T " std140 mismatch (expected " #SIZE")")

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

    struct RenderPerFrame
    {
        std::vector<RenderBufferHandle> m_uboHandles;
    };

    struct RenderMaterial
    {
        std::vector<RenderBufferHandle> m_uboHandles;
    };

    struct RenderMesh
    {
        RenderBufferHandle m_vertexBufferHandle{ 0 };
        RenderBufferHandle m_indexBufferHandle{ 0 };
        uint32_t m_indexCount{ 0 };
    };

    struct RenderList
    {
        RenderPerFrame m_frame;
        RenderMaterial m_material;
        RenderMesh m_mesh;
    };

    struct FrameUbo
    {
        alignas(16) glm::mat4 m_viewProj;
        alignas(16) glm::vec4 m_viewPos;            // xyz - pos, w - 1 always
        alignas(16) glm::vec4 m_lightDir;           // xyz - dir, w - 0(directional light)
        alignas(16) glm::vec4 m_lightColor;         // xyz - rgb, w - intensity
    };
    STD140_ASSERT(FrameUbo, 112);

    struct MaterialUbo
    {
        alignas(16) glm::vec4 m_albedo;
        alignas(16) glm::vec4 m_params;     // x - metallic, y - roughness, z - ao, w - padding
    };
    STD140_ASSERT(MaterialUbo, 32);
}