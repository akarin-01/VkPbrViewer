#pragma once

#include "resource/id_table.h"
#include "resource/handle.h"
#include "resource/resource_id.h"

#include <functional>
#include <unordered_map>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Key-addressed entries on top of IdTable: the same key always yields the
        /// same shared handle. GetOrCreate builds via the creator on miss,
        /// fires the reuser on hit.
        template <typename TKey, typename T, typename Hash = std::hash<TKey>>
        class KeyTable
        {
        public:
            using CreateFn = std::function<T(const TKey&)>;
            using ReuseFn = std::function<void(const TKey&)>;
            using DestroyFn = typename IdTable<T>::DestroyFn;

            explicit KeyTable(CreateFn creator,
                ReuseFn reuser = [](const TKey&) {},
                DestroyFn destroyer = [](T&&) {})
                : m_creator(std::move(creator)),
                m_reuser(std::move(reuser)),
                m_table(std::move(destroyer))
            {
            }

            ~KeyTable() = default;

            Handle<T> GetOrCreate(const TKey& key)
            {
                auto it = m_keyToId.find(key);
                if (it != m_keyToId.end() && m_table.Has(it->second))
                {
                    // Cache hit
                    m_reuser(key);
                    return m_table.GetShared(it->second);
                }

                // Cache miss, create new resource
                Handle<T> handle = m_table.Create(m_creator(key));
                m_keyToId[key] = handle.GetId();
                return handle;
            }

            T* Get(ResourceId id) { return m_table.Get(id); }
            const T* Get(ResourceId id) const { return m_table.Get(id); }

            size_t Size() const { return m_table.Size(); }

        private:
            CreateFn m_creator;
            ReuseFn m_reuser;
            IdTable<T> m_table;
            std::unordered_map<TKey, ResourceId, Hash> m_keyToId{};
        };
    }
}
