#pragma once

#include "rhi/constants.h"
#include "resource/constants.h"
#include "resource/handle.h"
#include "resource/resource_id.h"
#include "resource/resource_utils.h"
#include "resource/ubo.h"

#include <vulkan/vulkan.h>
#include <array>
#include <cstring>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        struct BufferData
        {
            VkBuffer m_buffer{ VK_NULL_HANDLE };
            VkDeviceMemory m_memory{ VK_NULL_HANDLE };
            void* m_mapped{ nullptr };
            VkDeviceSize m_size{ 0 };
        };

        struct TextureData
        {
            VkImage m_image{ VK_NULL_HANDLE };
            VkDeviceMemory m_memory{ VK_NULL_HANDLE };
            VkImageView m_imageView{ VK_NULL_HANDLE };
            VkSampler m_sampler{ VK_NULL_HANDLE };
            VkFormat m_format{ VK_FORMAT_UNDEFINED };
            VkExtent3D m_extent{ 0, 0, 1 };
            uint32_t m_mipLevels{ 1 };
            uint32_t m_arrayLayers{ 1 };
        };

        struct FrameData
        {
            std::array<BufferData, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};

            void Write(uint32_t frameIndex, const FrameUbo& ubo)
            {
                std::memcpy(m_ubos[frameIndex].m_mapped, &ubo, sizeof(ubo));
            }
        };

        struct ObjectData
        {
            std::array<BufferData, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};

            void Write(uint32_t frameIndex, const ObjectUbo& ubo)
            {
                std::memcpy(m_ubos[frameIndex].m_mapped, &ubo, sizeof(ubo));
            }
        };

        struct MeshData
        {
            MeshData() = default;
            explicit MeshData(const Rhi::Context& context) : m_context(&context) {}

            ~MeshData()
            {
                if (m_context)
                {
                    ResourceUtils::DestroyBufferData(*m_context, m_vertexBuffer);
                    ResourceUtils::DestroyBufferData(*m_context, m_indexBuffer);
                }
            }

            MeshData(const MeshData&) = delete;             // GPU buffers are not copyable
            MeshData& operator=(const MeshData&) = delete;
            MeshData(MeshData&&) = default;                 // owned by table entries
            MeshData& operator=(MeshData&&) = default;

            BufferData m_vertexBuffer{};
            BufferData m_indexBuffer{};
            uint32_t m_indexCount{ 0 };

        private:
            const Rhi::Context* m_context{ nullptr };       // destroy entry
        };

        struct MaterialData
        {
            std::array<Handle<TextureData>, Resource::kMaterialTextureCount> m_textures{};
            VkDescriptorSet m_set{ VK_NULL_HANDLE };        // Allocate new one when textures changed
        };

        struct EnvironmentData
        {
            Handle<TextureData> m_skyboxCubemap{};
            Handle<TextureData> m_irradiance{};
            Handle<TextureData> m_prefilter{};
            Handle<TextureData> m_brdfLut{};
            VkDescriptorSet m_set{ VK_NULL_HANDLE };        // Allocate new one when textures changed
        };

        struct MaterialDesc
        {
            std::array<ResourceId, kMaterialTextureCount> m_textureIds{};

            bool operator==(const MaterialDesc& other) const
            {
                return m_textureIds == other.m_textureIds;
            }
        };

        struct MaterialDescHash
        {
            size_t operator()(const MaterialDesc& desc) const
            {
                size_t h = 1469598103934665603ull;             // FNV-1a
                for (ResourceId id : desc.m_textureIds)
                {
                    h ^= static_cast<size_t>(id);
                    h *= 1099511628211ull;
                }
                return h;
            }
        };
    }
}
