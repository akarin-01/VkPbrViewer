#pragma once

#include "resource/handle_table.h"
#include "resource/handle.h"
#include "resource/resource_id.h"

#include <functional>
#include <unordered_map>

namespace Kita::Pbrv
{
    namespace Resource
    {
        template <typename TKey, typename T, typename Hash = std::hash<TKey>>
        class MappingTable
        {
        public:
            using CreateFn = std::function<T(const TKey&)>;
            using ReuseFn = std::function<void(const TKey&)>;
            using DestroyFn = typename HandleTable<T>::DestroyFn;

            explicit MappingTable(CreateFn creator,
                ReuseFn reuser = {},
                DestroyFn destroyer = {})
                : m_creator(std::move(creator)),
                m_reuser(std::move(reuser)),
                m_table(std::move(destroyer))
            {
            }

            ~MappingTable() = default;

            Handle<T> GetOrCreate(const TKey& key)
            {
                auto it = m_keyToId.find(key);
                if (it != m_keyToId.end() && m_table.Has(it->second))
                {
                    // Cache hit
                    if (m_reuser)
                    {
                        m_reuser(key);
                    }
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
            HandleTable<T> m_table;
            std::unordered_map<TKey, ResourceId, Hash> m_keyToId{};
        };
    }
}
