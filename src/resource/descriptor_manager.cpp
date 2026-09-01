#include "descriptor_manager.h"

#include "core/log.h"
#include "rhi/context.h"

#include <stdexcept>
#include <cassert>
#include <unordered_map>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace
        {
            std::string ToString(DescriptorManager::Type type)
            {
                switch (type)
                {
                case DescriptorManager::Type::Empty:        return "Empty";
                case DescriptorManager::Type::PerFrame:        return "PerFrame";
                case DescriptorManager::Type::PerMaterial:     return "PerMaterial";
                case DescriptorManager::Type::PerObject:       return "PerObject";
                case DescriptorManager::Type::PostProcess:     return "PostProcess";
                case DescriptorManager::Type::ComputeWrite:    return "ComputeWrite";
                case DescriptorManager::Type::ComputeSample:   return "ComputeSample";
                default: return "Unknown";
                }
            }
        }

        DescriptorSetPool::DescriptorSetPool(const Rhi::Context& context,
            const std::vector<BindingDesc>& bindingDescs,
            const std::string& name)
            : m_context(context), m_name(name)
        {
            // Layout
            {
                std::vector<VkDescriptorSetLayoutBinding> bindings(bindingDescs.size());
                for (size_t i = 0; i < bindings.size(); ++i)
                {
                    bindings[i].binding = static_cast<uint32_t>(i);
                    bindings[i].descriptorType = bindingDescs[i].m_type;
                    bindings[i].descriptorCount = bindingDescs[i].m_count;
                    bindings[i].stageFlags = bindingDescs[i].m_stage;
                }

                VkDescriptorSetLayoutCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
                createInfo.pBindings = bindings.data();

                if (vkCreateDescriptorSetLayout(m_context.Device(), &createInfo, nullptr, &m_layout) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create descriptor set layout!");
                }
            }

            // Pool sizes (same-type bindings accumulate into one entry)
            {
                std::unordered_map<VkDescriptorType, uint32_t> counts;
                for (const auto& desc : bindingDescs)
                {
                    counts[desc.m_type] += desc.m_count;
                }

                m_poolSizes.reserve(counts.size());
                for (const auto& [type, count] : counts)
                {
                    m_poolSizes.push_back({ type, count });
                }
            }

            // Pool
            m_pools.emplace_back(CreatePool());

            Core::Log::Info("[Resources] Create descriptor set pool: ", m_name,
                " (", bindingDescs.size(), " binding(s), capacity ", m_capacity, ")");
        }

        DescriptorSetPool::~DescriptorSetPool()
        {
            // Pools
            for (auto& pool : m_pools)
            {
                vkDestroyDescriptorPool(m_context.Device(), pool, nullptr);
            }

            // Layout
            vkDestroyDescriptorSetLayout(m_context.Device(), m_layout, nullptr);
        }

        VkDescriptorSet DescriptorSetPool::Allocate()
        {
            if (!m_freeSets.empty())
            {
                VkDescriptorSet recycled = m_freeSets.back();
                m_freeSets.pop_back();

                KITA_LOG_DEBUG("[Resources] Reuse descriptor set: ", m_name, " (",
                    m_freeSets.size(), " free)");
                return recycled;
            }

            if (m_used >= m_capacity)
            {
                KITA_LOG_DEBUG("[Resources] Descriptor pool full: ", m_name,
                    ", grow capacity ", m_capacity, " -> ", m_capacity * 2);

                m_used = 0;
                m_capacity *= 2;
                m_pools.emplace_back(CreatePool());
            }

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_pools.back();
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_layout;

            VkDescriptorSet set{};
            if (vkAllocateDescriptorSets(m_context.Device(), &allocInfo, &set) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate descriptor set: " + std::string(m_name));
            }

            ++m_used;

            KITA_LOG_DEBUG("[Resources] Allocate descriptor set: ", m_name,
                " (", m_used, "/", m_capacity, ")");
            return set;
        }

        void DescriptorSetPool::Recycle(VkDescriptorSet set)
        {
            m_freeSets.push_back(set);
        }

        VkDescriptorPool DescriptorSetPool::CreatePool() const
        {
            VkDescriptorPoolCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            createInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
            createInfo.pPoolSizes = m_poolSizes.data();
            createInfo.maxSets = m_capacity;

            VkDescriptorPool pool{};
            if (vkCreateDescriptorPool(m_context.Device(), &createInfo, nullptr, &pool) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create descriptor pool!");
            }

            return pool;
        }

        DescriptorManager::DescriptorManager(const Rhi::Context& context)
        {
            CreateSetPool(Type::Empty, context, {});

            CreateSetPool(Type::PerFrame, context,
                {
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                });

            CreateSetPool(Type::PerMaterial, context,
                {
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                });

            CreateSetPool(Type::PerObject, context,
                {
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
                });

            CreateSetPool(Type::PostProcess, context,
                {
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
                });

            CreateSetPool(Type::ComputeWrite, context,
                { { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT } });

            CreateSetPool(Type::ComputeSample, context,
                { { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT },
                  { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT } });
        }

        DescriptorManager::~DescriptorManager() = default;

        VkDescriptorSetLayout DescriptorManager::GetLayout(Type type) const
        {
            return GetPool(type).GetLayout();
        }

        VkDescriptorSet DescriptorManager::Allocate(Type type)
        {
            return GetPool(type).Allocate();
        }

        void DescriptorManager::Recycle(Type type, VkDescriptorSet set)
        {
            GetPool(type).Recycle(set);
        }

        void DescriptorManager::CreateSetPool(Type type, const Rhi::Context& context, const std::vector<BindingDesc>& bindingDescs)
        {
            auto& poolPtr = m_pools[static_cast<size_t>(type)];

            poolPtr = std::make_unique<DescriptorSetPool>(context, bindingDescs, ToString(type));
        }

        const DescriptorSetPool& DescriptorManager::GetPool(Type type) const
        {
            assert(type < Type::Count && "Descriptor layout type is invalid");

            auto& pool = m_pools[static_cast<size_t>(type)];
            assert(pool && "DescriptorSetPool not created for this layout type");

            return *pool;
        }

        DescriptorSetPool& DescriptorManager::GetPool(Type type)
        {
            return const_cast<DescriptorSetPool&>(static_cast<const DescriptorManager*>(this)->GetPool(type));
        }
    }
}
