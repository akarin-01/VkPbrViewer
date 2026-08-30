#pragma once

#include "rhi/constants.h"
#include "resource/resource_types.h"
#include "resource/gpu_layouts.h"

#include <vulkan/vulkan.h>
#include <array>
#include <cstring>

namespace Kita::Pbrv
{
    namespace Resource
    {
        struct PerObjectSet
        {
            // Set 3: K UBO slots + K descriptor sets, bound once at creation.
            std::array<Resource::UboResource, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};
            VkDescriptorSetLayout m_layout{ VK_NULL_HANDLE };

            void WriteData(uint32_t frameIndex, const Gpu::PerObject& data)
            {
                m_ubos[frameIndex].Write(&data, sizeof(data));
            }

            VkDescriptorSetLayout GetLayout() const { return m_layout; }
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }
        };
    }
}
