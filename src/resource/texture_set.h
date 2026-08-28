#pragma once

#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "resource/resources.h"
#include "resource/descriptor_manager.h"
#include "resource/render_texture.h"

#include <vulkan/vulkan.h>
#include <array>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Descriptor sets for a bundle of sampled textures.
        /// Owns all textures (including fallbacks); the caller only creates textures
        /// and passes them in. Sets are seeded at construction and stay valid forever.
        template <uint32_t SlotCount, uint32_t SetCount>
        class TextureSet
        {
        public:
            TextureSet(const Rhi::Context& context,
                Resources& resources,
                DescriptorManager& descriptorMgr,
                DescriptorLayoutType layoutType,
                const std::array<RenderTexture, SlotCount>& fallbacks)
                : m_context(context),
                m_resources(resources),
                m_descriptorMgr(descriptorMgr),
                m_layoutType(layoutType),
                m_fallbacks(fallbacks),
                m_textures(fallbacks)
            {
                // Sets
                for (auto& set : m_sets)
                {
                    set = m_descriptorMgr.Allocate(m_layoutType);
                    WriteSet(set);
                }
            }

            ~TextureSet()
            {
                // Sets will be destroyed automatically

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

            TextureSet(const TextureSet&) = delete;
            TextureSet& operator=(const TextureSet&) = delete;
            TextureSet(TextureSet&&) = delete;
            TextureSet& operator=(TextureSet&&) = delete;

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
            VkDescriptorSetLayout GetLayout() const { return m_descriptorMgr.GetLayout(m_layoutType); }
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
            const Rhi::Context& m_context;
            Resources& m_resources;
            DescriptorManager& m_descriptorMgr;

            // Count as the default: an unset layout type fails the manager's assert
            // instead of silently binding to a real layout
            DescriptorLayoutType m_layoutType{ DescriptorLayoutType::Count };

            std::array<RenderTexture, SlotCount> m_fallbacks{};
            std::array<RenderTexture, SlotCount> m_textures{};

            std::array<VkDescriptorSet, SetCount> m_sets{};
            uint32_t m_refreshCount{ 0 };
        };
    }
}
