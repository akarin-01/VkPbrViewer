#pragma once

#include "rhi/context.h"
#include "resource/resources.h"
#include "resource/descriptor_allocator.h"
#include "rhi/descriptor_writer.h"
#include "rhi/utils.h"
#include "resource/texture.h"

#include <vulkan/vulkan.h>
#include <array>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Descriptor sets for a bundle of sampled textures.
        /// Owns all textures (including fallbacks); the caller only creates textures
        /// and passes them in. Sets are seeded at construction and stay valid forever.
        template<uint32_t SlotCount, uint32_t SetCount>
        class RenderTextureSet
        {
        public:
            RenderTextureSet(const Rhi::RenderContext& context,
                RenderResources& resources,
                const DescriptorAllocator& descriptorAllocator,
                const std::array<RenderTexture, SlotCount>& fallbacks)
                : m_context(context),
                m_resources(resources),
                m_descriptorAllocator(descriptorAllocator),
                m_fallbacks(fallbacks),
                m_textures(fallbacks)
            {
                // Set layout
                {
                    std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
                    bindings[0].binding = 0;
                    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    bindings[0].descriptorCount = static_cast<uint32_t>(m_textures.size());
                    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                    VkDescriptorSetLayoutCreateInfo createInfo{};
                    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
                    createInfo.pBindings = bindings.data();

                    m_layout = Rhi::CreateDescriptorSetLayout(m_context.Device(), createInfo);
                }

                // Sets
                for (auto& set : m_sets)
                {
                    set = m_descriptorAllocator.Allocate(m_layout, "Texture set");
                    WriteSet(set);
                }
            }

            ~RenderTextureSet()
            {
                // Sets will be destroyed automatically

                // Set layout
                vkDestroyDescriptorSetLayout(m_context.Device(), m_layout, nullptr);

                // Textures
                for (uint32_t i = 0; i < SlotCount; ++i)
                {
                    if (m_textures[i] != m_fallbacks[i])
                    {
                        DestroyTexture(m_resources, m_textures[i]);
                    }

                    DestroyTexture(m_resources, m_fallbacks[i]);
                }
            }

            RenderTextureSet(const RenderTextureSet&) = delete;
            RenderTextureSet& operator=(const RenderTextureSet&) = delete;
            RenderTextureSet(RenderTextureSet&&) = delete;
            RenderTextureSet& operator=(RenderTextureSet&&) = delete;

            /// Replaces the changed slots. Old non-fallback textures are destroyed here.
            /// Empty slots in `textures` mean "keep current"; sets are written by RefreshSet.
            void Update(const std::array<RenderTexture, SlotCount>& textures)
            {
                bool changed = false;

                for (uint32_t i = 0; i < SlotCount; ++i)
                {
                    if (textures[i].IsEmpty())
                    {
                        continue;
                    }

                    if (m_textures[i] != textures[i])
                    {
                        if (m_textures[i] != m_fallbacks[i])
                        {
                            DestroyTexture(m_resources, m_textures[i]);
                        }

                        m_textures[i] = textures[i];
                        changed = true;
                    }
                }

                if (changed)
                {
                    m_refreshCount = SetCount;
                }
            }

            /// Call every frame; writes one set per frame while a refresh is pending.
            void RefreshSet(uint32_t frameIndex)
            {
                if (m_refreshCount == 0)
                {
                    return;
                }

                WriteSet(m_sets[frameIndex % SetCount]);
                --m_refreshCount;
            }

            const std::array<RenderTexture, SlotCount>& GetFallbacks() const { return m_fallbacks; }
            const std::array<RenderTexture, SlotCount>& GetTextures() const { return m_textures; }
            VkDescriptorSetLayout GetLayout() const { return m_layout; }
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex % SetCount]; }

        private:
            void WriteSet(VkDescriptorSet set)
            {
                Rhi::DescriptorWriter writer(m_resources, m_context.Device());
                for (uint32_t i = 0; i < SlotCount; ++i)
                {
                    auto& texture = m_textures[i];

                    writer.WriteImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.m_imageViewHandle, texture.m_samplerHandle);
                }

                writer.UpdateSet(set);
            }

        private:
            const Rhi::RenderContext& m_context;
            RenderResources& m_resources;
            const DescriptorAllocator& m_descriptorAllocator;

            std::array<RenderTexture, SlotCount> m_fallbacks{};
            std::array<RenderTexture, SlotCount> m_textures{};

            VkDescriptorSetLayout m_layout{ VK_NULL_HANDLE };
            std::array<VkDescriptorSet, SetCount> m_sets{};
            uint32_t m_refreshCount{ 0 };
        };
    }
}
