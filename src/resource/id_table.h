#pragma once

#include "resource/entry_table.h"
#include "resource/handle.h"
#include "resource/resource_id.h"

#include <functional>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Handle-shaped API over EntryTable: creates entries and produces
        /// refcounted handles. Handle constructs here only (IdTable is its friend).
        template <typename T>
        class IdTable
        {
        public:
            using DestroyFn = std::function<void(T&&)>;

            explicit IdTable(DestroyFn destroyer = [](T&&) {})
                : m_table(std::move(destroyer))
            {
            }

            ~IdTable() = default;

            Handle<T> Create(T&& res)
            {
                const ResourceId id = m_table.Add(std::move(res));
                return Handle<T>(id, &m_table);
            }

            /// Shared handle for an existing entry (AddRef +1); entry must exist.
            Handle<T> GetShared(ResourceId id)
            {
                m_table.AddRef(id);
                return Handle<T>(id, &m_table);
            }

            T* Get(ResourceId id) { return m_table.Get(id); }
            const T* Get(ResourceId id) const { return m_table.Get(id); }
            bool Has(ResourceId id) const { return m_table.Has(id); }
            void Release(ResourceId id) { m_table.Release(id); }
            size_t Size() const { return m_table.Size(); }

        private:
            EntryTable<T> m_table;
        };
    }
}
