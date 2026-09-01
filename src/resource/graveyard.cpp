#include "graveyard.h"

#include "core/log.h"
#include "rhi/context.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        Graveyard::Graveyard(const Rhi::Context& context, DescriptorManager& descriptorMgr)
            : m_context(context),
            m_descriptorMgr(descriptorMgr),
            m_memoryQueue([this](VkDeviceMemory& memory)
                {
                    vkFreeMemory(m_context.Device(), memory, nullptr);
                }),
            m_imageQueue([this](VkImage& image)
                {
                    vkDestroyImage(m_context.Device(), image, nullptr);
                }),
            m_bufferQueue([this](VkBuffer& buffer)
                {
                    vkDestroyBuffer(m_context.Device(), buffer, nullptr);
                }),
            m_samplerQueue([this](VkSampler& sampler)
                {
                    vkDestroySampler(m_context.Device(), sampler, nullptr);
                }),
            m_imageViewQueue([this](VkImageView& imageView)
                {
                    vkDestroyImageView(m_context.Device(), imageView, nullptr);
                }),
            m_descriptorSetQueue([this](DescriptorSetEntry& entry)
                {
                    m_descriptorMgr.Recycle(entry.m_layout, entry.m_set);
                })
        {
        }

        Graveyard::~Graveyard() = default;

        void Graveyard::Flush()
        {
            size_t descriptorSetCount = m_descriptorSetQueue.Flush();
            size_t imageViewCount = m_imageViewQueue.Flush();
            size_t samplerCount = m_samplerQueue.Flush();
            size_t bufferCount = m_bufferQueue.Flush();
            size_t imageCount = m_imageQueue.Flush();
            size_t memoryCount = m_memoryQueue.Flush();

            bool empty = (descriptorSetCount == 0 && imageViewCount == 0 && samplerCount == 0
                && bufferCount == 0 && imageCount == 0 && memoryCount == 0);
            if (!empty)
            {
                KITA_LOG_DEBUG("[Resource] Graveyard flush: ", descriptorSetCount, " descriptor sets, ",
                    imageViewCount, " image views, ",
                    samplerCount, " samplers, ", bufferCount, " buffers, ",
                    imageCount, " images, ", memoryCount, " memories");
            }
        }

        void Graveyard::PushMemory(VkDeviceMemory memory)
        {
            m_memoryQueue.Push(memory);
        }

        void Graveyard::PushBuffer(VkBuffer buffer)
        {
            m_bufferQueue.Push(buffer);
        }

        void Graveyard::PushImage(VkImage image)
        {
            m_imageQueue.Push(image);
        }

        void Graveyard::PushImageView(VkImageView imageView)
        {
            m_imageViewQueue.Push(imageView);
        }

        void Graveyard::PushSampler(VkSampler sampler)
        {
            m_samplerQueue.Push(sampler);
        }

        void Graveyard::PushDescriptorSet(VkDescriptorSet set, DescriptorManager::Type layout)
        {
            m_descriptorSetQueue.Push({ set, layout });
        }
    }
}
