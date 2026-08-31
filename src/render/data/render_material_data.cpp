#include "render_material_data.h"

#include "core/log.h"
#include "rhi/context.h"
#include "rhi/utils.h"
#include "rhi/descriptor_writer.h"
#include "resource/handle.h"
#include "resource/resources.h"
#include "resource/asset_types.h"
#include "resource/render_texture.h"
#include "resource/descriptor_manager.h"
#include "scene/material.h"

#include <glm/glm.hpp>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr Resource::DescriptorLayoutType kLayoutType = Resource::DescriptorLayoutType::PerMaterial;

            VkFormat ToFormat(Resource::TextureAsset::Type type)
            {
                switch (type)
                {
                case Resource::TextureAsset::Type::Srgb:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case Resource::TextureAsset::Type::Normal:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case Resource::TextureAsset::Type::MetallicRoughness:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case Resource::TextureAsset::Type::Linear:
                    return VK_FORMAT_R8_UNORM;
                default:
                    throw std::runtime_error("Invalid texture type!");
                }
            }

            const char* ToString(Resource::MaterialSlot slot)
            {
                switch (slot)
                {
                case Resource::MaterialSlot::Albedo:
                    return "Resource::Albedo";
                case Resource::MaterialSlot::Normal:
                    return "Resource::Normal";
                case Resource::MaterialSlot::MetallicRoughness:
                    return "Resource::MetallicRoughness";
                case Resource::MaterialSlot::AO:
                    return "Resource::AO";
                case Resource::MaterialSlot::Emissive:
                    return "Resource::Emissive";
                default:
                    return "Unknown";
                }
            }
        }

        RenderMaterialData::RenderMaterialData(const Rhi::Context& context,
            Resource::Resources& resources,
            Resource::DescriptorManager& descriptorMgr)
            : m_context(context),
            m_resources(resources),
            m_descriptorMgr(descriptorMgr)
        {
            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                m_fallbacks[i] = CreateFallback(static_cast<Resource::MaterialSlot>(i));
                m_textures[i] = m_fallbacks[i];
            }

            for (size_t i = 0; i < m_sets.size(); ++i)
            {
                m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                WriteSet(static_cast<uint32_t>(i));
            }
        }

        RenderMaterialData::~RenderMaterialData()
        {
            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                DestroyTextureSafe(i);
                Resource::DestroyTexture(m_resources, m_fallbacks[i]);
            }
        }

        void RenderMaterialData::UpdateTextures(const Scene::Material& sceneMat)
        {
            bool anyTexUpdated = false;

            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                const auto texHandle = sceneMat.GetTexture(static_cast<Resource::MaterialSlot>(i));
                if (texHandle.GetId() == m_lastSyncedTextureIds[i])
                {
                    continue;
                }
                m_lastSyncedTextureIds[i] = texHandle.GetId();

                DestroyTextureSafe(i);

                if (!texHandle.IsValid())
                {
                    m_textures[i] = m_fallbacks[i];
                }
                else
                {
                    m_textures[i] = CreateTexture(*texHandle);

                    Core::Log::Info("[Renderer] Create texture [", ToString(static_cast<Resource::MaterialSlot>(i)), "]: ",
                        texHandle->m_name, ", ", texHandle->GetByteCount(), " bytes");
                }

                anyTexUpdated = true;
            }

            if (anyTexUpdated)
            {
                m_setDirtyCount = Rhi::kMaxFramesInFlight;

                KITA_LOG_DEBUG("[Renderer] Update material descriptor set: textures changed");
            }
        }

        void RenderMaterialData::RefreshSet(uint32_t frameIndex)
        {
            if (m_setDirtyCount > 0)
            {
                WriteSet(frameIndex);
                --m_setDirtyCount;
            }
        }

        VkDescriptorSetLayout RenderMaterialData::GetSetLayout() const
        {
            return m_descriptorMgr.GetLayout(kLayoutType);
        }

        void RenderMaterialData::WriteSet(uint32_t frameIndex) const
        {
            Rhi::DescriptorWriter writer(m_resources, m_context.Device());
            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                writer.WriteImage(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_textures[i].m_imageViewHandle, m_textures[i].m_samplerHandle);
            }

            writer.UpdateSet(m_sets[frameIndex]);
        }

        void RenderMaterialData::DestroyTextureSafe(uint32_t slot)
        {
            // The fallback is shared: stored in m_fallbacks, it must survive
            // slot swaps to be restorable later.
            if (m_textures[slot] != m_fallbacks[slot])
            {
                Resource::DestroyTexture(m_resources, m_textures[slot]);
            }
        }

        Resource::RenderTexture RenderMaterialData::CreateTexture(const Resource::TextureAsset& texture) const
        {
            const VkFormat format = ToFormat(texture.m_type);
            const uint32_t mipLevels = Rhi::CalculateMipLevels(texture.m_width, texture.m_height);

            return Resource::Create2DTextureWithData(m_resources, texture.m_bytes.data(), texture.m_bytes.size(),
                texture.m_width, texture.m_height, format, mipLevels, m_resources.CreateSamplerLinearRepeatMip());
        }

        Resource::RenderTexture RenderMaterialData::CreateFallback(Resource::MaterialSlot slot) const
        {
            constexpr uint8_t white[] = { 255, 255, 255, 255 };
            constexpr uint8_t black[] = { 0, 0, 0, 255 };
            constexpr uint8_t flat[] = { 128, 128, 255, 255 };

            Resource::TextureAsset tex{};
            tex.m_name = "fallback";
            tex.m_width = 1;
            tex.m_height = 1;

            switch (slot)
            {
            case Resource::MaterialSlot::Albedo:
                tex.m_bytes = std::vector<uint8_t>(white, white + 4);
                tex.m_type = Resource::TextureAsset::Type::Srgb;
                break;
            case Resource::MaterialSlot::Normal:
                tex.m_bytes = std::vector<uint8_t>(flat, flat + 4);
                tex.m_type = Resource::TextureAsset::Type::Normal;
                break;
            case Resource::MaterialSlot::MetallicRoughness:
                tex.m_bytes = std::vector<uint8_t>(white, white + 4);
                tex.m_type = Resource::TextureAsset::Type::MetallicRoughness;
                break;
            case Resource::MaterialSlot::AO:
                tex.m_bytes = std::vector<uint8_t>(white, white + 1);
                tex.m_type = Resource::TextureAsset::Type::Linear;
                break;
            case Resource::MaterialSlot::Emissive:
                tex.m_bytes = std::vector<uint8_t>(black, black + 4);
                tex.m_type = Resource::TextureAsset::Type::Srgb;
                break;
            default:
                throw std::runtime_error("Invalid texture slot!");
            }

            return CreateTexture(tex);
        }
    }
}
