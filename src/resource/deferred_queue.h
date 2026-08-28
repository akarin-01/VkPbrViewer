#pragma once

#include "rhi/constants.h"

#include <functional>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Per-frame slots of entries awaiting destruction; Flush clears one slot.
        template <typename T>
        class DeferredQueue
        {
        public:
            using DestroyFn = std::function<void(T&)>;

            explicit DeferredQueue(DestroyFn destroyer = [](T&) {})
                : m_destroyer(destroyer)
            {
            }

            ~DeferredQueue()
            {
                for (uint32_t i = 0; i < Rhi::kMaxFramesInFlight; ++i)
                {
                    Flush(i);
                }
            }

            void Push(uint32_t frameIndex, std::unique_ptr<T> res)
            {
                m_queues[frameIndex].push_back(std::move(res));
            }

            size_t Flush(uint32_t frameIndex)
            {
                auto& queue = m_queues[frameIndex];
                size_t count = queue.size();
                for (auto& res : queue)
                {
                    m_destroyer(*res);
                }
                queue.clear();
                return count;
            }

        private:
            DestroyFn m_destroyer;
            std::array<std::vector<std::unique_ptr<T>>, Rhi::kMaxFramesInFlight> m_queues{};
        };
    }
}
