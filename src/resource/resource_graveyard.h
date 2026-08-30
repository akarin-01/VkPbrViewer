#pragma once

#include "rhi/constants.h"
#include "resource/resource_types.h"

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
        /// Holds dead GPU values and disposes of them K flushes later, where K
        /// is kMaxFramesInFlight: the fence observed at the K-th flush proves
        /// the death frame's submission complete.
        template <typename T>
        class GraveyardQueue
        {
        public:
            using DestroyFn = std::function<void(T&)>;

            explicit GraveyardQueue(DestroyFn destroyer = [](T&) {})
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

            void Push(T&& res)
            {
                m_pending.emplace_back(Entry{ Rhi::kMaxFramesInFlight, std::move(res) });
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
                T m_resource{};
            };

            std::vector<Entry> m_pending;
            DestroyFn m_destroyer;
        };

        /// Owns one GraveyardQueue per GPU value type; disposal never cascades
        /// back into the handle tables — entries are pure Vk values.
        class ResourceGraveyard
        {
        public:
            explicit ResourceGraveyard(const Rhi::Context& context);
            ~ResourceGraveyard();

            /// Once per app frame, after the frame's waitFence.
            void Flush();

            void PushBuffer(BufferResource&& buffer);
            void PushImage(ImageResource&& image);

        private:
            const Rhi::Context& m_context;

            GraveyardQueue<BufferResource> m_bufferQueue;
            GraveyardQueue<ImageResource> m_imageQueue;
        };
    }
}
