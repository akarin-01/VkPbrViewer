#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

#define STD140_ASSERT(T, SIZE)\
    static_assert(sizeof(T) == SIZE, #T " std140 mismatch (expected " #SIZE")")

namespace Kita::Pbrv
{
    using RenderBufferHandle = uint64_t;
    using RenderImageHandle = uint64_t;
    using RenderImageViewHandle = uint64_t;
    using RenderSamplerHandle = uint64_t;

    struct RenderBuffer
    {
        using Handle = RenderBufferHandle;
        VkBuffer m_buffer{ VK_NULL_HANDLE };
        VkDeviceMemory m_memory{ VK_NULL_HANDLE };
        void* m_mapped{ nullptr };
    };

    struct RenderImage
    {
        using Handle = RenderImageHandle;
        VkImage m_image{ VK_NULL_HANDLE };
        VkDeviceMemory m_memory{ VK_NULL_HANDLE };
    };

    struct RenderImageView
    {
        using Handle = RenderImageViewHandle;
        VkImageView m_imageView{ VK_NULL_HANDLE };
    };

    struct RenderSampler
    {
        using Handle = RenderSamplerHandle;
        VkSampler m_sampler{ VK_NULL_HANDLE };
    };

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

    struct RenderTexture
    {
        RenderImageHandle m_imageHandle{ 0 };
        RenderImageViewHandle m_imageViewHandle{ 0 };
    };

    struct RenderMaterial
    {
        std::vector<RenderBufferHandle> m_uboHandles;

        RenderTexture m_albedo;
        RenderSamplerHandle m_albedoSamplerHandle{ 0 };

        RenderTexture m_normal;
        RenderSamplerHandle m_normalSamplerHandle{ 0 };

        RenderTexture m_metallic;
        RenderSamplerHandle m_metallicSamplerHandle{ 0 };

        RenderTexture m_roughness;
        RenderSamplerHandle m_roughnessSamplerHandle{ 0 };

        RenderTexture m_ao;
        RenderSamplerHandle m_aoSamplerHandle{ 0 };
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