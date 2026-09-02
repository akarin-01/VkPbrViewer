#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        /// Simplified layout binding: descriptor index equals its position here.
        struct BindingDesc
        {
            VkDescriptorType m_type;
            uint32_t m_count;
            VkShaderStageFlags m_stage;
        };

        class DescriptorSetPool
        {
        public:
            DescriptorSetPool(const Rhi::Context& context,
                const std::vector<BindingDesc>& bindingDescs,
                const std::string& name);
            ~DescriptorSetPool();

            DescriptorSetPool(const DescriptorSetPool&) = delete;
            DescriptorSetPool& operator=(const DescriptorSetPool&) = delete;
            DescriptorSetPool(DescriptorSetPool&&) = delete;
            DescriptorSetPool& operator=(DescriptorSetPool&&) = delete;

            VkDescriptorSetLayout GetLayout() const { return m_layout; }
            VkDescriptorSet Allocate();
            void Recycle(VkDescriptorSet set);

        private:
            VkDescriptorPool CreatePool() const;

        private:
            const Rhi::Context& m_context;

            std::string m_name{ "default" };
            VkDescriptorSetLayout m_layout{ VK_NULL_HANDLE };
            std::vector<VkDescriptorPoolSize> m_poolSizes;
            std::vector<VkDescriptorPool> m_pools;
            std::vector<VkDescriptorSet> m_freeSets{};
            uint32_t m_capacity{ 8 };
            uint32_t m_used{ 0 };
        };

        class DescriptorManager
        {
        public:
            enum class Type : uint8_t
            {
                Empty,
                PerFrame,
                PerMaterial,
                PerObject,
                PostProcess,
                Lit,
                ComputeWrite,
                ComputeSample,
                Count
            };

            explicit DescriptorManager(const Rhi::Context& context);
            ~DescriptorManager();

            DescriptorManager(const DescriptorManager&) = delete;
            DescriptorManager& operator=(const DescriptorManager&) = delete;
            DescriptorManager(DescriptorManager&&) = delete;
            DescriptorManager& operator=(DescriptorManager&&) = delete;

            VkDescriptorSetLayout GetLayout(Type type) const;
            VkDescriptorSet Allocate(Type type);
            void Recycle(Type type, VkDescriptorSet set);

        private:
            void CreateSetPool(Type type,
                const Rhi::Context& context,
                const std::vector<BindingDesc>& bindingDescs);
            const DescriptorSetPool& GetPool(Type type) const;
            DescriptorSetPool& GetPool(Type type);

        private:
            std::array<std::unique_ptr<DescriptorSetPool>, static_cast<size_t>(Type::Count)> m_pools;
        };
    }
}
