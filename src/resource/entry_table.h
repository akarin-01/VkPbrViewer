#pragma once

#include "resource/resource_id.h"

#include <unordered_map>
#include <functional>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Reference-counted entry table: the backing store for Handle<T>.
        /// Add() creates an entry with count 1; Handle copies AddRef, Handle
        /// destruction Releases. Count 0 removes the entry and invokes the
        /// injected destroyer (immediate cleanup, or deferred transfer via
        /// MappingTable / MappingDeferredTable depending on the deployment).
        template <typename T>
        class EntryTable
        {
        public:
            using DestroyFn = std::function<void(T&&)>;

            explicit EntryTable(DestroyFn destroyer = [](T&&) {})
                : m_destroyer(std::move(destroyer))
            {
            }

            ~EntryTable()
            {
                for (auto& [id, entry] : m_entries)
                {
                    if (m_destroyer)
                    {
                        m_destroyer(std::move(entry.m_resource));
                    }
                }
            }

            ResourceId Add(T resource)
            {
                const ResourceId id = m_nextId++;
                m_entries.emplace(id, Entry{ std::move(resource), 1 });
                return id;
            }

            T* Get(ResourceId id)
            {
                return const_cast<T*>(static_cast<const EntryTable*>(this)->Get(id));
            }

            const T* Get(ResourceId id) const
            {
                auto it = m_entries.find(id);
                return it == m_entries.end() ? nullptr : &it->second.m_resource;
            }

            bool Has(ResourceId id) const
            {
                return m_entries.find(id) != m_entries.end();
            }

            void AddRef(ResourceId id)
            {
                auto it = m_entries.find(id);
                if (it != m_entries.end())
                {
                    ++it->second.m_refCount;
                }
            }

            void Release(ResourceId id)
            {
                auto it = m_entries.find(id);
                if (it == m_entries.end())
                {
                    return;
                }

                Entry& entry = it->second;
                if (--entry.m_refCount > 0)
                {
                    return;
                }

                auto resource = std::move(entry.m_resource);
                m_entries.erase(it);
                if (m_destroyer)
                {
                    m_destroyer(std::move(resource));
                }
            }

            size_t Size() const
            {
                return m_entries.size();
            }

        private:
            struct Entry
            {
                T m_resource;
                uint32_t m_refCount{ 0 };
            };

            DestroyFn m_destroyer;
            std::unordered_map<ResourceId, Entry> m_entries;
            ResourceId m_nextId{ kInvalidId + 1 };
        };
    }
}
