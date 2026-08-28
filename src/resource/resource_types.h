#pragma once

#include "rhi/constants.h"
#include "resource/constants.h"
#include "resource/handle.h"
#include "resource/resource_id.h"
#include "resource/ubo.h"

#include <vulkan/vulkan.h>
#include <array>
#include <cstring>

namespace Kita::Pbrv
{
    namespace Resource
    {
        struct BufferResource
        {
            VkBuffer m_buffer{ VK_NULL_HANDLE };
            VkDeviceMemory m_memory{ VK_NULL_HANDLE };
            void* m_mapped{ nullptr };
            VkDeviceSize m_size{ 0 };
        };

        struct TextureResource
        {
            using Handle = Resource::Handle<TextureResource>;

            VkImage m_image{ VK_NULL_HANDLE };
            VkDeviceMemory m_memory{ VK_NULL_HANDLE };
            VkImageView m_imageView{ VK_NULL_HANDLE };
            VkSampler m_sampler{ VK_NULL_HANDLE };
            VkFormat m_format{ VK_FORMAT_UNDEFINED };
            VkExtent3D m_extent{ 0, 0, 1 };
            uint32_t m_mipLevels{ 1 };
            uint32_t m_arrayLayers{ 1 };
        };

        struct FrameResource
        {
            using Handle = Resource::Handle<FrameResource>;

            std::array<BufferResource, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};

            void Write(uint32_t frameIndex, const FrameUbo& ubo)
            {
                std::memcpy(m_ubos[frameIndex].m_mapped, &ubo, sizeof(ubo));
            }
        };

        struct ObjectResource
        {
            using Handle = Resource::Handle<ObjectResource>;

            std::array<BufferResource, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};

            void Write(uint32_t frameIndex, const ObjectUbo& ubo)
            {
                std::memcpy(m_ubos[frameIndex].m_mapped, &ubo, sizeof(ubo));
            }
        };

        struct MeshResource
        {
            using Handle = Resource::Handle<MeshResource>;

            BufferResource m_vertexBuffer{};
            BufferResource m_indexBuffer{};
            uint32_t m_indexCount{ 0 };

            VkBuffer GetVertexBuffer() const { return m_vertexBuffer.m_buffer; }
            VkBuffer GetIndexBuffer() const { return m_indexBuffer.m_buffer; }
            uint32_t GetIndexCount() const { return m_indexCount; }
        };

        struct MaterialResource
        {
            using Handle = Resource::Handle<MaterialResource>;

            std::array<TextureResource::Handle, kMaterialTextureCount> m_textures{};
            VkDescriptorSet m_set{ VK_NULL_HANDLE };        // Allocate new one when textures changed
        };

        struct EnvironmentResource
        {
            using Handle = Resource::Handle<EnvironmentResource>;

            TextureResource::Handle m_skyboxCubemap{};
            TextureResource::Handle m_irradiance{};
            TextureResource::Handle m_prefilter{};
            TextureResource::Handle m_brdfLut{};
            VkDescriptorSet m_set{ VK_NULL_HANDLE };        // Allocate new one when textures changed
        };

        struct MaterialDesc
        {
            std::array<ResourceId, kMaterialTextureCount> m_textureIds{};

            bool operator==(const MaterialDesc& other) const
            {
                return m_textureIds == other.m_textureIds;
            }

            struct Hash
            {
                size_t operator()(const MaterialDesc& desc) const
                {
                    size_t h = 1469598103934665603ull;
                    for (ResourceId id : desc.m_textureIds)
                    {
                        h ^= static_cast<size_t>(id);
                        h *= 1099511628211ull;
                    }
                    return h;
                }
            };
        };
    }
}
