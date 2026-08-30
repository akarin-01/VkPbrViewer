#pragma once

#include "rhi/constants.h"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <functional>
#include <vector>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        /// Holds dead raw Vulkan handles and disposes of them K flushes later,
        /// where K is kMaxFramesInFlight: the fence observed at the K-th flush
        /// proves the death frame's submission complete.
        template <typename THandle>
        class GraveyardQueue
        {
        public:
            using DestroyFn = std::function<void(THandle&)>;

            explicit GraveyardQueue(DestroyFn destroyer = [](THandle&) {})
                : m_destroyer(std::move(destroyer))
            {
            }

            ~GraveyardQueue()
            {
                for (auto& entry : m_pending)
                {
                    m_destroyer(entry.m_resource);
                }
            }

            void Push(THandle handle)
            {
                m_pending.emplace_back(Entry{ Rhi::kMaxFramesInFlight, handle });
            }

            /// Decrements every entry's lifetime; zeros are disposed. Call
            /// once per app frame, after the frame's waitFence.
            size_t Flush()
            {
                std::vector<Entry> survivors;
                size_t destroyCount = 0;

                for (auto& entry : m_pending)
                {
                    if (--entry.m_lifetime == 0)
                    {
                        m_destroyer(entry.m_resource);
                        ++destroyCount;
                        continue;
                    }

                    survivors.push_back(std::move(entry));
                }

                m_pending = std::move(survivors);
                return destroyCount;
            }

        private:
            struct Entry
            {
                uint64_t m_lifetime{ Rhi::kMaxFramesInFlight };
                THandle m_resource{};
            };

            std::vector<Entry> m_pending;
            DestroyFn m_destroyer;
        };

        /// Owns one GraveyardQueue per Vulkan handle type. Every entry is a
        /// raw handle, so disposal never cascades back into the handle tables;
        /// composed resources take themselves apart (Release) before pushing.
        class Graveyard
        {
        public:
            explicit Graveyard(const Rhi::Context& context);
            ~Graveyard();

            /// Once per app frame, after the frame's waitFence.
            void Flush();

            void PushMemory(VkDeviceMemory memory);
            void PushBuffer(VkBuffer buffer);
            void PushImage(VkImage image);
            void PushImageView(VkImageView imageView);
            void PushSampler(VkSampler sampler);

        private:
            const Rhi::Context& m_context;

            // Declaration order = destruction order (reversed at teardown):
            // memory last, because vkFreeMemory requires that no bound
            // buffer/image outlives it; views/samplers die first. Flush order
            // is explicit in Flush() and follows the same dependency chain.
            GraveyardQueue<VkDeviceMemory> m_memoryQueue;
            GraveyardQueue<VkImage> m_imageQueue;
            GraveyardQueue<VkBuffer> m_bufferQueue;
            GraveyardQueue<VkSampler> m_samplerQueue;
            GraveyardQueue<VkImageView> m_imageViewQueue;
        };
    }
}
