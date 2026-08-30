#include "resource_utils.h"

#include "rhi/context.h"
#include "rhi/one_shot_command.h"
#include "rhi/utils.h"
#include "resource/resource_types.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace
        {
            BufferRhi CreateBufferHelper(const Rhi::Context& context, const BufferDesc& desc)
            {
                BufferRhi buffer{};
                buffer.m_size = desc.m_size;

                VkBufferCreateInfo bufferInfo{};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = desc.m_size;
                bufferInfo.usage = desc.m_usage;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                if (vkCreateBuffer(context.Device(), &bufferInfo, nullptr, &buffer.m_buffer) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create buffer!");
                }

                VkMemoryRequirements req{};
                vkGetBufferMemoryRequirements(context.Device(), buffer.m_buffer, &req);

                VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                allocInfo.allocationSize = req.size;
                allocInfo.memoryTypeIndex = Rhi::FindMemoryType(context.PhysicalDevice(), req.memoryTypeBits, desc.m_properties);
                if (vkAllocateMemory(context.Device(), &allocInfo, nullptr, &buffer.m_memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to allocate buffer memory!");
                }

                vkBindBufferMemory(context.Device(), buffer.m_buffer, buffer.m_memory, 0);

                if (desc.m_mapped)
                {
                    vkMapMemory(context.Device(), buffer.m_memory, 0, desc.m_size, 0, &buffer.m_mapped);
                }

                return buffer;
            }

            ImageRhi CreateImageHelper(const Rhi::Context& context, const ImageDesc& desc)
            {
                ImageRhi image{};
                image.m_format = desc.m_format;
                image.m_extent = desc.m_extent;
                image.m_mipLevels = desc.m_mipLevels;
                image.m_arrayLayers = desc.m_arrayLayers;
                image.m_aspectMask = desc.m_aspectMask;

                // Image
                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = desc.m_type;
                imageInfo.flags = desc.m_flags;
                imageInfo.extent = desc.m_extent;
                imageInfo.mipLevels = desc.m_mipLevels;
                imageInfo.arrayLayers = desc.m_arrayLayers;
                imageInfo.format = desc.m_format;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = desc.m_usage;
                imageInfo.samples = desc.m_samples;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                if (vkCreateImage(context.Device(), &imageInfo, nullptr, &image.m_image) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create image!");
                }

                // Memory
                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements(context.Device(), image.m_image, &memRequirements);

                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex = Rhi::FindMemoryType(context.PhysicalDevice(), memRequirements.memoryTypeBits, desc.m_properties);

                if (vkAllocateMemory(context.Device(), &allocInfo, nullptr, &image.m_memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to allocate image memory!");
                }

                vkBindImageMemory(context.Device(), image.m_image, image.m_memory, 0);

                return image;
            }

            /// Immediate destruction for one-shot buffers (staging); never deferred.
            void DestroyBufferImmediate(const Rhi::Context& context, BufferRhi& buffer)
            {
                if (buffer.m_mapped)
                {
                    vkUnmapMemory(context.Device(), buffer.m_memory);
                    buffer.m_mapped = nullptr;
                }
                vkDestroyBuffer(context.Device(), buffer.m_buffer, nullptr);
                vkFreeMemory(context.Device(), buffer.m_memory, nullptr);
                buffer = {};
            }

            /// Copy `size` bytes between buffers.
            void CopyBuffer(VkCommandBuffer commandBuffer,
                const BufferRhi& src, const BufferRhi& dst, VkDeviceSize size)
            {
                VkBufferCopy copy{};
                copy.size = size;
                vkCmdCopyBuffer(commandBuffer, src.m_buffer, dst.m_buffer, 1, &copy);
            }

            /// Upload `region` into the image's base mip (image must be in TRANSFER_DST).
            void CopyBufferToImage(VkCommandBuffer commandBuffer,
                const BufferRhi& src, const ImageRhi& dst, const VkBufferImageCopy& region)
            {
                vkCmdCopyBufferToImage(commandBuffer, src.m_buffer, dst.m_image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            }
        }

        namespace ResourceUtils
        {
            BufferRhi CreateBufferRhi(const Rhi::Context& context,
                BufferDesc desc, const void* data, size_t size)
            {
                assert((data == nullptr) == (size == 0) && "CreateBuffer: data and size must agree");

                if (!data)
                {
                    // Non-data buffer
                    return CreateBufferHelper(context, desc);
                }

                const bool hostVisible = (desc.m_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

                if (hostVisible)
                {
                    // Host-visible: map and write directly into memory
                    assert(desc.m_mapped && "CreateBuffer: host visible must map");

                    BufferRhi buffer = CreateBufferHelper(context, desc);
                    std::memcpy(buffer.m_mapped, data, size);
                    return buffer;
                }

                // Device-local: add the transfer flag and upload through a staging buffer.
                desc.m_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                BufferRhi buffer = CreateBufferHelper(context, desc);

                BufferDesc stagingDesc{};
                stagingDesc.m_size = size;
                stagingDesc.m_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                stagingDesc.m_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                stagingDesc.m_mapped = true;
                BufferRhi staging = CreateBufferRhi(context, stagingDesc, data, size);

                {
                    Rhi::OneShotCommand cmd(context);
                    CopyBuffer(cmd.Handle(), staging, buffer, size);
                }

                DestroyBufferImmediate(context, staging);

                return buffer;
            }

            ImageRhi CreateImageRhi(const Rhi::Context& context,
                ImageDesc desc, const void* data, size_t size)
            {
                assert((data == nullptr) == (size == 0) && "CreateImage: data and size must agree");

                if (!data)
                {
                    // Non-data image
                    return CreateImageHelper(context, desc);
                }

                // Device-local: add the transfer flag and upload through a staging buffer.
                desc.m_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                ImageRhi image = CreateImageHelper(context, desc);

                BufferDesc stagingDesc{};
                stagingDesc.m_size = size;
                stagingDesc.m_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                stagingDesc.m_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                stagingDesc.m_mapped = true;
                BufferRhi staging = CreateBufferRhi(context, stagingDesc, data, size);

                {
                    Rhi::OneShotCommand cmd(context);

                    // Full image: UNDEFINED -> TRANSFER_DST (all mips/layers writable)
                    TransitionImageLayout(cmd.Handle(), image,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

                    // Base mip: tightly packed upload of the full extent (layer 0)
                    VkBufferImageCopy region{};
                    region.bufferOffset = 0;
                    region.bufferRowLength = 0;
                    region.bufferImageHeight = 0;
                    region.imageSubresource.aspectMask = desc.m_aspectMask;
                    region.imageSubresource.mipLevel = 0;
                    region.imageSubresource.baseArrayLayer = 0;
                    region.imageSubresource.layerCount = 1;
                    region.imageOffset = { 0, 0, 0 };
                    region.imageExtent = desc.m_extent;
                    CopyBufferToImage(cmd.Handle(), staging, image, region);

                    if (desc.m_mipLevels > 1)
                    {
                        // Mip chain; ends with every subresource in SHADER_READ_ONLY.
                        GenerateImageMipmaps(cmd.Handle(), image);
                    }
                    else
                    {
                        TransitionImageLayout(cmd.Handle(), image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
                    }
                }

                DestroyBufferImmediate(context, staging);

                return image;
            }

            ImageViewRhi CreateImageViewRhi(const Rhi::Context& context,
                const ImageRhi& image, const ImageViewDesc& desc)
            {
                ImageViewRhi imageView{};

                VkImageViewCreateInfo imageViewInfo{};
                imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewInfo.image = image.m_image;
                imageViewInfo.viewType = desc.m_type;
                imageViewInfo.format = image.m_format;
                imageViewInfo.subresourceRange.aspectMask = image.m_aspectMask;
                if (desc.m_fullRange)
                {
                    imageViewInfo.subresourceRange.baseMipLevel = 0;
                    imageViewInfo.subresourceRange.levelCount = image.m_mipLevels;
                    imageViewInfo.subresourceRange.baseArrayLayer = 0;
                    imageViewInfo.subresourceRange.layerCount = image.m_arrayLayers;
                }
                else
                {
                    imageViewInfo.subresourceRange.baseMipLevel = desc.m_baseMipLevel;
                    imageViewInfo.subresourceRange.levelCount = desc.m_levelCount;
                    imageViewInfo.subresourceRange.baseArrayLayer = desc.m_baseArrayLayer;
                    imageViewInfo.subresourceRange.layerCount = desc.m_layerCount;
                }

                if (vkCreateImageView(context.Device(), &imageViewInfo, nullptr, &imageView.m_imageView) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create image view!");
                }

                return imageView;
            }

            SamplerRhi CreateSamplerRhi(const Rhi::Context& context,
                const SamplerDesc& desc)
            {
                SamplerRhi sampler{};

                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

                // Filter
                samplerInfo.magFilter = desc.m_magFilter;
                samplerInfo.minFilter = desc.m_minFilter;

                // Address
                samplerInfo.addressModeU = desc.m_addressModeU;
                samplerInfo.addressModeV = desc.m_addressModeV;
                samplerInfo.addressModeW = desc.m_addressModeW;

                samplerInfo.unnormalizedCoordinates = VK_FALSE;
                samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                samplerInfo.compareEnable = VK_FALSE;
                samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

                // Anisotropy
                if (desc.m_anisotropy)
                {
                    samplerInfo.anisotropyEnable = VK_TRUE;
                    samplerInfo.maxAnisotropy = context.MaxAnisotropy();
                }
                else
                {
                    samplerInfo.anisotropyEnable = VK_FALSE;
                    samplerInfo.maxAnisotropy = 1.0f;
                }

                // Mipmode
                samplerInfo.mipLodBias = 0.0f;
                samplerInfo.minLod = 0.0f;

                switch (desc.m_mipMode)
                {
                case SamplerDesc::MipMode::None:
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    samplerInfo.maxLod = 0.0f;
                    break;
                case SamplerDesc::MipMode::Nearest:
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
                    break;
                case SamplerDesc::MipMode::Linear:
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
                    break;
                default:
                    throw std::runtime_error("Invalid mip mode!");
                }

                if (vkCreateSampler(context.Device(), &samplerInfo, nullptr, &sampler.m_sampler) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create sampler!");
                }

                return sampler;
            }

            uint32_t CalculateMipLevels(uint32_t width, uint32_t height)
            {
                return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);
            }

            void TransitionImageLayout(VkCommandBuffer commandBuffer, const ImageRhi& image,
                VkImageLayout oldLayout, VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask)
            {
                VkImageSubresourceRange range{};
                range.aspectMask = image.m_aspectMask;
                range.baseMipLevel = 0;
                range.levelCount = image.m_mipLevels;
                range.baseArrayLayer = 0;
                range.layerCount = image.m_arrayLayers;

                TransitionImageLayout(commandBuffer, image, oldLayout, newLayout,
                    srcStageMask, srcAccessMask, dstStageMask, dstAccessMask, range);
            }

            void TransitionImageLayout(VkCommandBuffer commandBuffer, const ImageRhi& image,
                VkImageLayout oldLayout, VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
                const VkImageSubresourceRange& range)
            {
                VkImageMemoryBarrier2 barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                barrier.srcStageMask = srcStageMask;
                barrier.srcAccessMask = srcAccessMask;
                barrier.dstStageMask = dstStageMask;
                barrier.dstAccessMask = dstAccessMask;
                barrier.oldLayout = oldLayout;
                barrier.newLayout = newLayout;
                barrier.image = image.m_image;
                barrier.subresourceRange = range;

                VkDependencyInfo dependencyInfo{};
                dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                dependencyInfo.dependencyFlags = 0;
                dependencyInfo.imageMemoryBarrierCount = 1;
                dependencyInfo.pImageMemoryBarriers = &barrier;

                vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
            }

            void GenerateImageMipmaps(VkCommandBuffer commandBuffer, const ImageRhi& image,
                VkImageLayout finalLayout, VkPipelineStageFlags2 finalStageMask)
            {
                const int32_t width = static_cast<int32_t>(image.m_extent.width);
                const int32_t height = static_cast<int32_t>(image.m_extent.height);

                // Mip 0 (just uploaded) becomes the blit source.
                VkImageSubresourceRange srcRange{};
                srcRange.aspectMask = image.m_aspectMask;
                srcRange.baseMipLevel = 0;
                srcRange.levelCount = 1;
                srcRange.baseArrayLayer = 0;
                srcRange.layerCount = image.m_arrayLayers;

                TransitionImageLayout(commandBuffer, image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                    srcRange);

                for (uint32_t mip = 1; mip < image.m_mipLevels; ++mip)
                {
                    VkImageSubresourceRange dstRange{};
                    dstRange.aspectMask = image.m_aspectMask;
                    dstRange.baseMipLevel = mip;
                    dstRange.levelCount = 1;
                    dstRange.baseArrayLayer = 0;
                    dstRange.layerCount = image.m_arrayLayers;

                    // UNDEFINED -> TRANSFER_DST: contents are overwritten by the blit.
                    TransitionImageLayout(commandBuffer, image,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE,
                        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                        dstRange);

                    VkImageBlit blit{};
                    blit.srcSubresource.aspectMask = image.m_aspectMask;
                    blit.srcSubresource.mipLevel = mip - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = image.m_arrayLayers;
                    blit.srcOffsets[0] = { 0, 0, 0 };
                    blit.srcOffsets[1] = { std::max(1, width >> (mip - 1)), std::max(1, height >> (mip - 1)), 1 };
                    blit.dstSubresource.aspectMask = image.m_aspectMask;
                    blit.dstSubresource.mipLevel = mip;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount = image.m_arrayLayers;
                    blit.dstOffsets[0] = { 0, 0, 0 };
                    blit.dstOffsets[1] = { std::max(1, width >> mip), std::max(1, height >> mip), 1 };
                    vkCmdBlitImage(commandBuffer, image.m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        image.m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

                    // This mip becomes the source for the next one.
                    TransitionImageLayout(commandBuffer, image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                        dstRange);
                }

                // Uniform final layout across all mips, ordered for the next consumer.
                VkImageSubresourceRange allRange{};
                allRange.aspectMask = image.m_aspectMask;
                allRange.baseMipLevel = 0;
                allRange.levelCount = image.m_mipLevels;
                allRange.baseArrayLayer = 0;
                allRange.layerCount = image.m_arrayLayers;

                TransitionImageLayout(commandBuffer, image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, finalLayout,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                    finalStageMask, VK_ACCESS_2_SHADER_READ_BIT,
                    allRange);
            }
        }
    }
}
