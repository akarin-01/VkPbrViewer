#pragma once

#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>

#define STD140_ASSERT(T, SIZE)\
    static_assert(sizeof(T) == SIZE, #T " std140 mismatch (expected " #SIZE")")

namespace Kita::Pbrv
{
    struct FrameUbo
    {
        alignas(16) glm::mat4 m_viewProj;
        alignas(16) glm::mat4 m_skyboxViewProj;
        alignas(16) glm::vec4 m_viewPos;            // xyz - pos, w - 1 always
        alignas(16) glm::vec4 m_lightDir;           // xyz - dir, w - 0(directional light)
        alignas(16) glm::vec4 m_lightColor;         // xyz - rgb, w - intensity
    };
    STD140_ASSERT(FrameUbo, 176);

    struct MaterialPC
    {
        alignas(16) glm::vec4 m_albedo;
        alignas(16) glm::vec4 m_params;             // x - metallic, y - roughness, z - ao, w - padding
    };
    STD140_ASSERT(MaterialPC, 32);

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
        RenderSamplerHandle m_samplerHandle{ 0 };

        bool operator==(const RenderTexture& other) const
        {
            return (m_imageHandle == other.m_imageHandle)
                && (m_imageViewHandle == other.m_imageViewHandle)
                && (m_samplerHandle == other.m_samplerHandle);
        }

        bool operator!=(const RenderTexture& other) const
        {
            return !(*this == other);
        }
    };

    struct RenderMaterial
    {
        MaterialPC m_pushConstant{ {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };

        std::array<RenderTexture, kMaterialTextureCount> m_textures{};
        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_sets{};
        uint32_t m_setRefreshCount{ 0 };
    };

    struct RenderMesh
    {
        RenderBufferHandle m_vertexBufferHandle{ 0 };
        RenderBufferHandle m_indexBufferHandle{ 0 };
        uint32_t m_indexCount{ 0 };
    };

    struct RenderSkybox
    {
        RenderTexture m_texture{};
        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_sets{};
        uint32_t m_setRefreshCount{ 0 };
    };

    struct RenderList
    {
        RenderPerFrame m_frame{};
        RenderMaterial m_material{};
        RenderMesh m_mesh{};
        RenderSkybox m_skybox{};
    };
}
