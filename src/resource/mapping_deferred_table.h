#pragma once

#include "resource/mapping_table.h"
#include "resource/deferred_queue.h"
#include "resource/handle.h"

#include <functional>
#include <cstdint>
#include <memory>

namespace Kita::Pbrv
{
    namespace Resource
    {
        template <typename TKey, typename T, typename Hash = std::hash<TKey>>
        class MappingDeferredTable
        {
        public:
            using CreateFn = typename MappingTable<TKey, T, Hash>::CreateFn;
            using ReuseFn = typename MappingTable<TKey, T, Hash>::ReuseFn;
            using DestroyFn = typename DeferredQueue<T>::DestroyFn;

            explicit MappingDeferredTable(CreateFn creator,
                ReuseFn reuser = [](const TKey&) {},
                DestroyFn destroyer = [](T&) {})
                : m_table(creator, reuser, [this](T&& res)
                    {
                        m_queue.Push(m_frameIndex, std::make_unique<T>(std::move(res)));
                    }),
                m_queue(destroyer)
            {
            }

            ~MappingDeferredTable() = default;

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
            MappingTable<TKey, T, Hash> m_table;
            DeferredQueue<T> m_queue;
            uint32_t m_frameIndex{ 0 };
        };
    }
}
