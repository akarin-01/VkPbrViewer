#pragma once

#include "resource/resource_id.h"
#include "resource/resource_table.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        template <typename T>
        class Handle
        {
        public:
            Handle();
            ~Handle();
            Handle(const Handle& other);
            Handle& operator=(const Handle& other);
            Handle(Handle&& other) noexcept;
            Handle& operator=(Handle&& other) noexcept;

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

            const T* Get() const;
            T* Get();

            const T* operator->() const
            {
                return Get();
            }
            T* operator->()
            {
                return Get();
            }

            const T& operator*() const
            {
                return *Get();
            }
            T& operator*()
            {
                return *Get();
            }

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
            friend class AssetManager;

            Handle(ResourceId id, ResourceTable<T>* table);

        private:
            ResourceId m_id{ kInvalidId };
            ResourceTable<T>* m_table{ nullptr };
        };

        template <typename T>
        inline Handle<T>::Handle() = default;

        template <typename T>
        inline Handle<T>::~Handle()
        {
            if (IsValid())
            {
                m_table->Release(m_id);
            }
        }

        template <typename T>
        inline Handle<T>::Handle(const Handle& other)
            : m_id(other.m_id), m_table(other.m_table)
        {
            if (IsValid())
            {
                m_table->AddRef(m_id);
            }
        }

        template <typename T>
        inline Handle<T>& Handle<T>::operator=(const Handle& other)
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

        template <typename T>
        inline Handle<T>::Handle(Handle&& other) noexcept
            : m_id(other.m_id), m_table(other.m_table)
        {
            other.m_id = kInvalidId;
            other.m_table = nullptr;
        }

        template <typename T>
        inline Handle<T>& Handle<T>::operator=(Handle&& other) noexcept
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

        template <typename T>
        inline const T* Handle<T>::Get() const
        {
            return IsValid() ? m_table->Get(m_id) : nullptr;
        }

        template <typename T>
        inline T* Handle<T>::Get()
        {
            return const_cast<T*>(static_cast<const Handle*>(this)->Get());
        }

        template <typename T>
        inline Handle<T>::Handle(ResourceId id, ResourceTable<T>* table)
            : m_id(id), m_table(table)
        {
        }
    }
}
