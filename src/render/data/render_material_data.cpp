#include "render_material_data.h"
#include "core/log.h"
#include "rhi/context.h"
#include "resource/handle.h"
#include "resource/resources.h"
#include "resource/texture.h"
#include "resource/texture_set.h"
#include "scene/material.h"

#include <glm/glm.hpp>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            VkFormat ToFormat(Resource::TextureType type)
            {
                switch (type)
                {
                case Resource::TextureType::Srgb:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case Resource::TextureType::Normal:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case Resource::TextureType::MetallicRoughness:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case Resource::TextureType::Linear:
                    return VK_FORMAT_R8_UNORM;
                default:
                    throw std::runtime_error("Invalid texture type!");
                }
            }

            const char* ToString(Resource::MaterialTextureSlot tex)
            {
                switch (tex)
                {
                case Resource::Albedo:
                    return "Resource::Albedo";
                case Resource::Normal:
                    return "Resource::Normal";
                case Resource::MetallicRoughness:
                    return "Resource::MetallicRoughness";
                case Resource::AO:
                    return "Resource::AO";
                case Resource::Emissive:
                    return "Resource::Emissive";
                default:
                    return "Unknown";
                }
            }
        }

        RenderMaterialData::RenderMaterialData(const Rhi::RenderContext& context,
            Resource::RenderResources& resources,
            const Resource::DescriptorAllocator& descriptorAllocator)
            : m_context(context),
            m_resources(resources),
            m_descriptorAllocator(descriptorAllocator)
        {
            m_textureSet = std::make_unique<TextureSet>(m_context, m_resources, m_descriptorAllocator,
                TextureArray{ CreateFallbacks() });
        }

        RenderMaterialData::~RenderMaterialData()
        {
            m_textureSet.reset();
        }

        void RenderMaterialData::Update(uint32_t frameIndex, const Scene::Material& sceneMat)
        {
            m_pushConstant.m_albedo = sceneMat.GetAlbedo();
            m_pushConstant.m_params = glm::vec4(sceneMat.GetMetallic(),
                sceneMat.GetRoughness(),
                sceneMat.GetAO(),
                0.0f);
            m_pushConstant.m_emissive = glm::vec4(sceneMat.GetEmissive(), 1.0f);

            bool anyTexUpdated = false;
            TextureArray updatedTexs{};
            for (uint32_t i = 0; i < Resource::kMaterialTextureCount; ++i)
            {
                const auto texHandle = sceneMat.GetTexture(i);
                if (texHandle.GetId() != m_lastSyncedTextureIds[i])
                {
                    m_lastSyncedTextureIds[i] = texHandle.GetId();

                    if (!texHandle.IsValid())
                    {
                        updatedTexs[i] = m_textureSet->GetFallbacks()[i];
                    }
                    else
                    {
                        updatedTexs[i] = CreateTexture(*texHandle);

                        Core::Log::Info("[Renderer] Create texture [", ToString(Resource::MaterialTextureSlot(i)), "]: ",
                            texHandle->m_name, ", ", texHandle->GetByteCount(), " bytes");
                    }

                    anyTexUpdated = true;
                }
            }

            if (anyTexUpdated)
            {
                m_textureSet->Update(updatedTexs);
                KITA_LOG_DEBUG("[Renderer] Update material descriptor set: textures changed");
            }

            m_textureSet->RefreshSet(frameIndex);
        }

        VkDescriptorSetLayout RenderMaterialData::GetSetLayout() const
        {
            return m_textureSet->GetLayout();
        }

        const VkDescriptorSet& RenderMaterialData::GetSet(uint32_t frameIndex) const
        {
            return m_textureSet->GetSet(frameIndex);
        }

        RenderMaterialData::TextureArray RenderMaterialData::CreateFallbacks() const
        {
            constexpr uint8_t white[] = { 255, 255, 255, 255 };
            constexpr uint8_t black[] = { 0, 0, 0, 255 };
            constexpr uint8_t flat[] = { 128, 128, 255, 255 };
            constexpr uint32_t width = 1;
            constexpr uint32_t height = 1;

            TextureArray fallbacks{};

            // Albedo: white sRGB
            {
                Resource::Texture tex{};
                tex.m_name = "fallback";
                tex.m_bytes = std::vector<uint8_t>(white, white + 4);
                tex.m_width = width;
                tex.m_height = height;
                tex.m_type = Resource::TextureType::Srgb;
                fallbacks[Resource::Albedo] = CreateTexture(tex);
            }
            // Normal: flat (128, 128, 255)
            {
                Resource::Texture tex{};
                tex.m_name = "fallback";
                tex.m_bytes = std::vector<uint8_t>(flat, flat + 4);
                tex.m_width = width;
                tex.m_height = height;
                tex.m_type = Resource::TextureType::Normal;
                fallbacks[Resource::Normal] = CreateTexture(tex);
            }
            // MR: white (metallic 255, roughness 255)
            {
                Resource::Texture tex{};
                tex.m_name = "fallback";
                tex.m_bytes = std::vector<uint8_t>(white, white + 4);
                tex.m_width = width;
                tex.m_height = height;
                tex.m_type = Resource::TextureType::MetallicRoughness;
                fallbacks[Resource::MetallicRoughness] = CreateTexture(tex);
            }
            // AO: white, single channel
            {
                Resource::Texture tex{};
                tex.m_name = "fallback";
                tex.m_bytes = std::vector<uint8_t>(white, white + 1);
                tex.m_width = width;
                tex.m_height = height;
                tex.m_type = Resource::TextureType::Linear;
                fallbacks[Resource::AO] = CreateTexture(tex);
            }
            // Emissive: black
            {
                Resource::Texture tex{};
                tex.m_name = "fallback";
                tex.m_bytes = std::vector<uint8_t>(black, black + 4);
                tex.m_width = width;
                tex.m_height = height;
                tex.m_type = Resource::TextureType::Srgb;
                fallbacks[Resource::Emissive] = CreateTexture(tex);
            }

            return fallbacks;
        }

        Resource::RenderTexture RenderMaterialData::CreateTexture(const Resource::Texture& texture) const
        {
            const VkFormat format = ToFormat(texture.m_type);
            const uint32_t mipLevels = Rhi::CalculateMipLevels(texture.m_width, texture.m_height);

            return Resource::Create2DTextureWithData(m_resources, texture.m_bytes.data(), texture.m_bytes.size(),
                texture.m_width, texture.m_height, format, mipLevels, m_resources.CreateSamplerLinearRepeatMip());
        }
    }
}
