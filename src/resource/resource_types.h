#pragma once

#include "resource/descriptor_manager.h"
#include "rhi/constants.h"
#include "resource/constants.h"
#include "resource/gpu_layouts.h"
#include "resource/handle.h"
#include "resource/resource_id.h"

#include <array>
#include <cstring>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Resource
    {
        // ------------- L0: Rhi (vk objects) -------------
        // RAII + deferred destroy: must be used via handle

        struct BufferDesc
        {
            VkDeviceSize m_size{ 0 };
            VkBufferUsageFlags m_usage{ 0 };
            VkMemoryPropertyFlags m_properties{ 0 };
            bool m_mapped{ false };
        };

        struct BufferRhi
        {
            using Handle = Handle<BufferRhi>;

            VkBuffer m_buffer{ VK_NULL_HANDLE };
            VkDeviceMemory m_memory{ VK_NULL_HANDLE };
            void* m_mapped{ nullptr };
            VkDeviceSize m_size{ 0 };
        };

        struct ImageDesc
        {
            VkImageType m_type{ VK_IMAGE_TYPE_2D };
            VkImageCreateFlags m_flags{ 0 };
            VkExtent3D m_extent{ 0, 0, 1 };
            uint32_t m_mipLevels{ 1 };
            uint32_t m_arrayLayers{ 1 };
            VkFormat m_format{ VK_FORMAT_UNDEFINED };
            VkImageAspectFlags m_aspectMask{ VK_IMAGE_ASPECT_NONE };
            VkImageUsageFlags m_usage{ 0 };
            VkSampleCountFlagBits m_samples{ VK_SAMPLE_COUNT_1_BIT };
            VkMemoryPropertyFlags m_properties{ 0 };
        };

        struct ImageRhi
        {
            using Handle = Handle<ImageRhi>;

            VkImage m_image{ VK_NULL_HANDLE };
            VkDeviceMemory m_memory{ VK_NULL_HANDLE };
            VkExtent3D m_extent{ 0, 0, 1 };
            uint32_t m_mipLevels{ 1 };
            uint32_t m_arrayLayers{ 1 };
            VkFormat m_format{ VK_FORMAT_UNDEFINED };
            VkImageAspectFlags m_aspectMask{ VK_IMAGE_ASPECT_NONE };
        };

        struct ImageViewDesc
        {
            VkImageViewType m_type{ VK_IMAGE_VIEW_TYPE_2D };
            bool m_fullRange{ true };
            uint32_t m_baseMipLevel{ 0 };
            uint32_t m_levelCount{ 1 };
            uint32_t m_baseArrayLayer{ 0 };
            uint32_t m_layerCount{ 1 };
        };

        struct ImageViewRhi
        {
            using Handle = Handle<ImageViewRhi>;

            VkImageView m_imageView{ VK_NULL_HANDLE };
            ImageRhi::Handle m_image{};            // Image must be destroyed after image view
        };

        struct SamplerDesc
        {
            enum class MipMode
            {
                None,       // single level: maxLod = 0
                Nearest,    // full chain, NEAREST mip filter
                Linear,     // full chain, LINEAR mip filter
            };

            VkFilter m_magFilter{ VK_FILTER_LINEAR };
            VkFilter m_minFilter{ VK_FILTER_LINEAR };
            MipMode m_mipMode{ MipMode::Linear };
            VkSamplerAddressMode m_addressModeU{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
            VkSamplerAddressMode m_addressModeV{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
            VkSamplerAddressMode m_addressModeW{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
            bool m_anisotropy{ false };

            bool operator==(const SamplerDesc& other) const
            {
                return m_magFilter == other.m_magFilter
                    && m_minFilter == other.m_minFilter
                    && m_mipMode == other.m_mipMode
                    && m_addressModeU == other.m_addressModeU
                    && m_addressModeV == other.m_addressModeV
                    && m_addressModeW == other.m_addressModeW
                    && m_anisotropy == other.m_anisotropy;
            }

            struct Hash
            {
                size_t operator()(const SamplerDesc& d) const
                {
                    size_t h = 1469598103934665603ull;
                    auto mix = [&h](uint64_t v) { h ^= v; h *= 1099511628211ull; };
                    mix(static_cast<uint64_t>(d.m_magFilter));
                    mix(static_cast<uint64_t>(d.m_minFilter));
                    mix(static_cast<uint64_t>(d.m_mipMode));
                    mix(static_cast<uint64_t>(d.m_addressModeU));
                    mix(static_cast<uint64_t>(d.m_addressModeV));
                    mix(static_cast<uint64_t>(d.m_addressModeW));
                    mix(d.m_anisotropy ? 1u : 0u);
                    return h;
                }
            };
        };

        struct SamplerRhi
        {
            using Handle = Handle<SamplerRhi>;

            VkSampler m_sampler{ VK_NULL_HANDLE };
        };

        struct DescriptorSetRhi
        {
            using Handle = Handle<DescriptorSetRhi>;
            using Type = DescriptorManager::Type;

            VkDescriptorSet m_set{ VK_NULL_HANDLE };
            Type m_layout{ Type::Count };

            const VkDescriptorSet& GetSet() const { return m_set; }
        };

        // ------------- L1: Resource (render resources) -------------

        struct MeshResource
        {
            using Handle = Handle<MeshResource>;

            BufferRhi::Handle m_vertexBuffer{};
            BufferRhi::Handle m_indexBuffer{};
            uint32_t m_indexCount{ 0 };

            VkBuffer GetVertexBuffer() const { return m_vertexBuffer->m_buffer; }
            VkBuffer GetIndexBuffer() const { return m_indexBuffer->m_buffer; }
            uint32_t GetIndexCount() const { return m_indexCount; }
        };

        struct UboResource
        {
            BufferRhi::Handle m_ubo{};

            void Write(const void* data, size_t size) const
            {
                std::memcpy(m_ubo->m_mapped, data, size);
            }

            VkBuffer GetBuffer() const { return m_ubo->m_buffer; }
        };

        struct TextureResource
        {
            ImageRhi::Handle m_image{};
            ImageViewRhi::Handle m_imageView{};
            SamplerRhi::Handle m_sampler{};

            VkImage GetImage() const { return m_image->m_image; }
            VkImageView GetImageView() const { return m_imageView->m_imageView; }
            VkSampler GetSampler() const { return m_sampler->m_sampler; }
            VkFormat GetFormat() const { return m_image->m_format; }
            bool IsEmpty() const { return !m_image; }
        };

        struct TargetDesc
        {
            VkExtent2D m_extent{ 0, 0 };
            VkFormat m_colorFormat{ VK_FORMAT_UNDEFINED };
            VkFormat m_depthFormat{ VK_FORMAT_UNDEFINED };
            VkSampleCountFlagBits m_msaaSamples{ VK_SAMPLE_COUNT_1_BIT };
            SamplerDesc m_samplerDesc{};
        };

        struct TargetResource
        {
            TextureResource m_colorTexture{};
            TextureResource m_resolveTexture{};
            TextureResource m_depthTexture{};

            VkImage GetColorImage() const { return m_colorTexture.GetImage(); }
            VkImageView GetColorImageView() const { return m_colorTexture.GetImageView(); }
            VkSampler GetColorSampler() const { return m_colorTexture.GetSampler(); }
            VkFormat GetColorFormat() const { return m_colorTexture.GetFormat(); }

            VkImage GetResolveImage() const { return m_resolveTexture.GetImage(); }
            VkImageView GetResolveImageView() const { return m_resolveTexture.GetImageView(); }
            VkSampler GetResolveSampler() const { return m_resolveTexture.GetSampler(); }

            VkImage GetDepthImage() const { return m_depthTexture.GetImage(); }
            VkImageView GetDepthImageView() const { return m_depthTexture.GetImageView(); }
            VkSampler GetDepthSampler() const { return m_depthTexture.GetSampler(); }
            VkFormat GetDepthFormat() const { return m_depthTexture.GetFormat(); }
        };

        // ------------- L2: Set (descriptor sets) -------------

        struct PerObjectSet
        {
            // Set 3: K UBO slots + K descriptor sets, bound once at creation.
            std::array<UboResource, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<DescriptorSetRhi::Handle, Rhi::kMaxFramesInFlight> m_sets{};

            void WriteData(uint32_t frameIndex, const Gpu::PerObject& data)
            {
                m_ubos[frameIndex].Write(&data, sizeof(data));
            }

            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]->GetSet(); }
        };

        struct MaterialDesc
        {
            // One texture id per material slot; kInvalidId = fallback
            std::array<ResourceId, kMaterialSlotCount> m_textureIds{};

            bool operator==(const MaterialDesc& other) const
            {
                return m_textureIds == other.m_textureIds;
            }

            struct Hash
            {
                size_t operator()(const MaterialDesc& d) const
                {
                    size_t h = 1469598103934665603ull;
                    auto mix = [&h](uint64_t v) { h ^= v; h *= 1099511628211ull; };
                    for (ResourceId id : d.m_textureIds)
                    {
                        mix(id);
                    }
                    return h;
                }
            };
        };

        struct PerMaterialSet
        {
            using Handle = Handle<PerMaterialSet>;

            // Set 2 (one immutable set; empty slots share the manager's fallbacks).
            std::array<TextureResource, kMaterialSlotCount> m_textures{};
            DescriptorSetRhi::Handle m_set{};

            const VkDescriptorSet& GetSet() const { return m_set->GetSet(); }
        };

        struct PostProcessSet
        {
            // Set 1: K UBO slots + K descriptor sets, bound once at creation.
            std::array<UboResource, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<DescriptorSetRhi::Handle, Rhi::kMaxFramesInFlight> m_sets{};

            void WriteData(uint32_t frameIndex, const Gpu::PostProcess& data)
            {
                m_ubos[frameIndex].Write(&data, sizeof(data));
            }

            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]->GetSet(); }
        };

        struct PerFrameSet
        {
            // Set 0: K UBO slots + K descriptor sets; IBL textures fixed at creation.
            std::array<UboResource, Rhi::kMaxFramesInFlight> m_ubos{};
            TextureResource m_skybox{};
            TextureResource m_irradiance{};
            TextureResource m_prefilter{};
            std::array<DescriptorSetRhi::Handle, Rhi::kMaxFramesInFlight> m_sets{};

            void WriteData(uint32_t frameIndex, const Gpu::PerFrame& data)
            {
                m_ubos[frameIndex].Write(&data, sizeof(data));
            }

            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]->GetSet(); }
        };
    }
}
