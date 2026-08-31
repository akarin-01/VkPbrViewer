#pragma once

#include "resource/resource_id.h"

#include <functional>
#include <stdexcept>
#include <unordered_map>

namespace Kita::Pbrv
{
    namespace Resource
    {
        template <typename T>
        class Handle;
        template <typename T>
        class HandleTable;

        /// Refcounted entry store; the leaf of the handle system.
        template <typename T>
        class RefTable
        {
        public:
            using DestroyFn = std::function<void(T&&)>;

            explicit RefTable(DestroyFn destroyer = [](T&&) {})
                : m_destroyer(std::move(destroyer))
            {
            }

            ~RefTable()
            {
                for (auto& [id, entry] : m_entries)
                {
                    m_destroyer(std::move(entry.m_resource));
                }
            }

            const T* Get(ResourceId id) const
            {
                auto it = m_entries.find(id);
                return it == m_entries.end() ? nullptr : &it->second.m_resource;
            }

            T* Get(ResourceId id)
            {
                return const_cast<T*>(static_cast<const RefTable*>(this)->Get(id));
            }

            bool Has(ResourceId id) const
            {
                return m_entries.find(id) != m_entries.end();
            }

            size_t Size() const
            {
                return m_entries.size();
            }

            /// Insert an ownerless entry (refcount 0); the handle machinery
            /// adopts it via AddRef.
            ResourceId Add(T resource)
            {
                const ResourceId id = m_nextId++;
                m_entries.emplace(id, Entry{ std::move(resource), 0 });
                return id;
            }

        private:
            friend class Handle<T>;

            /// Refcount mutators; only the handle machinery may call them.
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
                m_destroyer(std::move(resource));
            }

            struct Entry
            {
                T m_resource;
                uint32_t m_refCount{ 0 };
            };

            DestroyFn m_destroyer;
            std::unordered_map<ResourceId, Entry> m_entries;
            ResourceId m_nextId{ kInvalidId + 1 };
        };

        /// RAII refcount handle: invalid (default) or valid. A valid handle
        /// holds a ref, so its entry is always alive.
        template <typename T>
        class Handle
        {
        public:
            Handle() = default;

            ~Handle()
            {
                if (IsValid())
                {
                    m_table->Release(m_id);
                }
            }

            Handle(const Handle& other)
                : m_id(other.m_id), m_table(other.m_table)
            {
                if (IsValid())
                {
                    m_table->AddRef(m_id);
                }
            }

            Handle& operator=(const Handle& other)
            {
                if (this != &other)
                {
                    if (IsValid())
                    {
                        m_table->Release(m_id);
                    }

                    m_id = other.m_id;
                    m_table = other.m_table;

                    if (IsValid())
                    {
                        m_table->AddRef(m_id);
                    }
                }

                return *this;
            }

            Handle(Handle&& other) noexcept
                : m_id(other.m_id), m_table(other.m_table)
            {
                other.m_id = kInvalidId;
                other.m_table = nullptr;
            }

            Handle& operator=(Handle&& other) noexcept
            {
                if (this != &other)
                {
                    if (IsValid())
                    {
                        m_table->Release(m_id);
                    }

                    m_id = other.m_id;
                    m_table = other.m_table;

                    other.m_id = kInvalidId;
                    other.m_table = nullptr;
                }
                return *this;
            }

            bool IsValid() const
            {
                return m_id != kInvalidId && m_table != nullptr;
            }

            explicit operator bool() const
            {
                return IsValid();
            }

            ResourceId GetId() const
            {
                return m_id;
            }

            const T* Get() const
            {
                return IsValid() ? m_table->Get(m_id) : nullptr;
            }

            T* Get()
            {
                return const_cast<T*>(static_cast<const Handle*>(this)->Get());
            }

            const T* operator->() const { return Get(); }
            T* operator->() { return Get(); }
            const T& operator*() const { return *Get(); }
            T& operator*() { return *Get(); }

            void Reset()
            {
                if (IsValid())
                {
                    m_table->Release(m_id);

                    m_id = kInvalidId;
                    m_table = nullptr;
                }
            }

        private:
            friend class HandleTable<T>;

            /// Adopts a ref: every handle born from the store or another
            /// handle adds one on arrival.
            Handle(ResourceId id, RefTable<T>* table)
                : m_id(id), m_table(table)
            {
                if (IsValid())
                {
                    m_table->AddRef(m_id);
                }
            }

        private:
            ResourceId m_id{ kInvalidId };
            RefTable<T>* m_table{ nullptr };
        };

        /// Facade over the store: mints Handles; the only surface callers touch.
        template <typename T>
        class HandleTable
        {
        public:
            using DestroyFn = typename RefTable<T>::DestroyFn;

            explicit HandleTable(DestroyFn destroyer = [](T&&) {})
                : m_store(std::move(destroyer))
            {
            }

            ~HandleTable() = default;

            /// Mints a handle over a new entry.
            Handle<T> Create(T&& res)
            {
                const ResourceId id = m_store.Add(std::move(res));
                return Handle<T>(id, &m_store);
            }

            /// Shared handle for an existing entry (+1 ref); entry must exist.
            Handle<T> GetShared(ResourceId id)
            {
                if (!m_store.Has(id))
                {
                    throw std::runtime_error("GetShared: entry does not exist");
                }
                return Handle<T>(id, &m_store);
            }

            T* Get(ResourceId id) { return m_store.Get(id); }
            const T* Get(ResourceId id) const { return m_store.Get(id); }
            bool Has(ResourceId id) const { return m_store.Has(id); }
            size_t Size() const { return m_store.Size(); }

        private:
            RefTable<T> m_store;
        };
    }
}
