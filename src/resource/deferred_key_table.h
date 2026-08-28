#pragma once

#include "resource/key_table.h"
#include "resource/deferred_queue.h"
#include "resource/handle.h"

#include <functional>
#include <cstdint>
#include <memory>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Key-addressed entries with deferred destruction: a dead entry (count 0)
        /// is queued and destroyed on FlushDeferred — call it after the frame's
        /// waitFence.
        template <typename TKey, typename T, typename Hash = std::hash<TKey>>
        class DeferredKeyTable
        {
        public:
            using CreateFn = typename KeyTable<TKey, T, Hash>::CreateFn;
            using ReuseFn = typename KeyTable<TKey, T, Hash>::ReuseFn;
            using DestroyFn = typename DeferredQueue<T>::DestroyFn;

            explicit DeferredKeyTable(CreateFn creator,
                ReuseFn reuser = [](const TKey&) {},
                DestroyFn destroyer = [](T&) {})
                : m_table(creator, reuser, [this](T&& res)
                    {
                        m_queue.Push(m_frameIndex, std::make_unique<T>(std::move(res)));
                    }),
                m_queue(destroyer)
            {
            }

            ~DeferredKeyTable() = default;

            Handle<T> GetOrCreate(const TKey& key)
            {
                return m_table.GetOrCreate(key);
            }

            void FlushDeferred(uint32_t frameIndex)
            {
                m_frameIndex = frameIndex;
                m_queue.Flush(frameIndex);
            }

        private:
            KeyTable<TKey, T, Hash> m_table;
            DeferredQueue<T> m_queue;
            uint32_t m_frameIndex{ 0 };
        };
    }
}
