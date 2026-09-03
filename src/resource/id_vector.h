#pragma once

#include "resource/resource_id.h"

#include <unordered_map>
#include <utility>
#include <vector>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Dense id-keyed vector.
        /// T must expose GetId(). Use FindOrAdd(id, factory) when T cannot be
        /// constructed from ResourceId alone.
        template <typename T>
        class IdVector
        {
        public:
            /// Returns the existing element, or creates a new one with `create(id)`.
            template <typename TCreator>
            T& FindOrAdd(ResourceId id, TCreator&& create)
            {
                auto it = m_index.find(id);
                if (it != m_index.end())
                {
                    return m_items[it->second];
                }

                const size_t index = m_items.size();
                m_items.emplace_back(std::forward<TCreator>(create)(id));
                m_index.emplace(id, index);
                return m_items.back();
            }

            /// Returns the existing element, or creates a new one with `T(id)`.
            T& FindOrAdd(ResourceId id)
            {
                return FindOrAdd(id, [](ResourceId id) { return T(id); });
            }

            void Remove(ResourceId id)
            {
                auto it = m_index.find(id);
                if (it == m_index.end())
                {
                    return;
                }

                const size_t index = it->second;
                m_index.erase(it);

                if (index + 1 < m_items.size())
                {
                    std::swap(m_items[index], m_items.back());
                    m_index[m_items[index].GetId()] = index;
                }
                m_items.pop_back();
            }

            const T* Find(ResourceId id) const
            {
                auto it = m_index.find(id);
                return it == m_index.end() ? nullptr : &m_items[it->second];
            }

            T* Find(ResourceId id)
            {
                return const_cast<T*>(static_cast<const IdVector*>(this)->Find(id));
            }

            bool Contains(ResourceId id) const
            {
                return m_index.find(id) != m_index.end();
            }

            const std::vector<T>& Items() const { return m_items; }
            std::vector<T>& Items() { return m_items; }

            size_t Size() const { return m_items.size(); }
            void Clear() { m_index.clear(); m_items.clear(); }

        private:
            std::vector<T> m_items;
            std::unordered_map<ResourceId, size_t> m_index;
        };
    }
}
