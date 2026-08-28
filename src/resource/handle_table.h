#pragma once

#include "resource/ref_table.h"
#include "resource/handle.h"
#include "resource/resource_id.h"

#include <functional>
#include <cassert>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Mints refcounted Handle<T>s over RefTable: Create inserts an entry
        /// and returns its first handle, GetShared AddRefs an existing entry.
        /// Handle's constructor is private; HandleTable is its only friend.
        template <typename T>
        class HandleTable
        {
        public:
            using DestroyFn = std::function<void(T&&)>;

            explicit HandleTable(DestroyFn destroyer = [](T&&) {})
                : m_table(std::move(destroyer))
            {
            }

            ~HandleTable() = default;

            Handle<T> Create(T&& res)
            {
                const ResourceId id = m_table.Add(std::move(res));
                return Handle<T>(id, &m_table);
            }

            /// Shared handle for an existing entry (AddRef +1); entry must exist.
            Handle<T> GetShared(ResourceId id)
            {
                assert(m_table.Has(id) && "GetShared: entry does not exist");
                m_table.AddRef(id);
                return Handle<T>(id, &m_table);
            }

            T* Get(ResourceId id) { return m_table.Get(id); }
            const T* Get(ResourceId id) const { return m_table.Get(id); }
            bool Has(ResourceId id) const { return m_table.Has(id); }
            void Release(ResourceId id) { m_table.Release(id); }
            size_t Size() const { return m_table.Size(); }

        private:
            RefTable<T> m_table;
        };
    }
}
