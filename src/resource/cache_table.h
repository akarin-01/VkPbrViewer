#pragma once

#include "core/log.h"
#include "resource/handle.h"
#include "resource/resource_id.h"

#include <optional>
#include <unordered_map>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Dedup over HandleTable: hit on the id map, stale ids rebuilt.
        /// The creator is a pure function (key -> optional resource);
        /// nullopt is never cached. Create is the direct (uncached) path.
        template <typename T, typename TKey, typename THash = std::hash<TKey>>
        class CacheTable
        {
        public:
            using DestroyFn = typename HandleTable<T>::DestroyFn;

            explicit CacheTable(const char* label, DestroyFn destroyer = [](T&&) {})
                : m_label(label),
                m_table(std::move(destroyer))
            {
            }

            ~CacheTable() = default;

            /// Direct (uncached) creation; returns the entry's first handle.
            Handle<T> Create(T&& res)
            {
                return m_table.Create(std::move(res));
            }

            /// Cached creation: shared handle on hit, otherwise runs
            /// `create(key)` and inserts the resource (unless nullopt).
            template <typename TCreator>
            Handle<T> GetOrCreate(const TKey& key, TCreator&& create)
            {
                auto it = m_ids.find(key);
                if (it != m_ids.end())
                {
                    const ResourceId id = it->second;
                    // A stale id (entry released) falls through to a rebuild.
                    if (m_table.Has(id))
                    {
                        // Cache hit: add ref
                        KITA_LOG_DEBUG("[Resource] Reuse ", m_label);
                        return m_table.GetShared(id);
                    }
                }

                // Cache miss: build and insert; nullopt is never cached.
                std::optional<T> resource = create(key);
                if (!resource)
                {
                    return Handle<T>();
                }
                Handle<T> handle = m_table.Create(std::move(*resource));
                m_ids[key] = handle.GetId();
                return handle;
            }

            T* Get(ResourceId id) { return m_table.Get(id); }
            const T* Get(ResourceId id) const { return m_table.Get(id); }
            size_t Size() const { return m_table.Size(); }

        private:
            const char* m_label;
            std::unordered_map<TKey, ResourceId, THash> m_ids;   // outlives m_table
            HandleTable<T> m_table;
        };
    }
}
