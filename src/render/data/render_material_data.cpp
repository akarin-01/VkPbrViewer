#include "render_material_data.h"

#include "core/log.h"
#include "scene/material.h"
#include "scene/texture.h"
#include "render/render_context.h"
#include "render/render_resources.h"
#include "render/render_texture_set.h"

#include <glm/glm.hpp>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace
    {
        VkFormat ToFormat(TextureType type)
        {
            switch (type)
            {
            case TextureType::Srgb:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureType::Normal:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureType::MetallicRoughness:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureType::Linear:
                return VK_FORMAT_R8_UNORM;
            default:
                throw std::runtime_error("Invalid texture type!");
            }
        }

        const char* ToString(MaterialTextureSlot tex)
        {
            switch (tex)
            {
            case Albedo:
                return "Albedo";
            case Normal:
                return "Normal";
            case MetallicRoughness:
                return "MetallicRoughness";
            case AO:
                return "AO";
            case Emissive:
                return "Emissive";
            default:
                return "Unknown";
            }
        }

        const Texture& GetTexture(const Material& mat, uint32_t slot)
        {
            switch (slot)
            {
            case Albedo:
                return mat.GetAlbedoTex();
            case Normal:
                return mat.GetNormalTex();
            case MetallicRoughness:
                return mat.GetMRTex();
            case AO:
                return mat.GetAOTex();
            case Emissive:
                return mat.GetEmissiveTex();
            default:
                throw std::runtime_error("Invalid texture slot!");
            }
        }
    }

    RenderMaterialData::RenderMaterialData(const RenderContext& context,
        RenderResources& resources,
        const DescriptorAllocator& descriptorAllocator)
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

    void RenderMaterialData::Update(uint32_t frameIndex, const Material& sceneMat)
    {
        m_pushConstant.m_albedo = sceneMat.GetAlbedo();
        m_pushConstant.m_params = glm::vec4(sceneMat.GetMetallic(),
            sceneMat.GetRoughness(),
            sceneMat.GetAO(),
            0.0f);
        m_pushConstant.m_emissive = glm::vec4(sceneMat.GetEmissive(), 1.0f);

        bool anyTexUpdated = false;
        TextureArray updatedTexs{};
        for (uint32_t i = 0; i < kMaterialTextureCount; ++i)
        {
            auto& sceneTex = GetTexture(sceneMat, i);
            if (m_lastSyncedRevisions[i] != sceneTex.GetRevision())
            {
                m_lastSyncedRevisions[i] = sceneTex.GetRevision();

                if (sceneTex.IsEmpty())
                {
                    updatedTexs[i] = m_textureSet->GetFallbacks()[i];
                }
                else
                {
                    updatedTexs[i] = CreateTexture(sceneTex);

                    Log::Info("[Renderer] Create texture [", ToString(MaterialTextureSlot(i)), "]: ",
                        sceneTex.GetName(), ", ", sceneTex.GetPixelCount(), " bytes");
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

        // Albedo
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(white, white + 4), width, height, TextureType::Srgb);
            fallbacks[Albedo] = CreateTexture(tex);
        }
        // Normal
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(flat, flat + 4), width, height, TextureType::Normal);
            fallbacks[Normal] = CreateTexture(tex);
        }
        // MR
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(white, white + 4), width, height, TextureType::MetallicRoughness);
            fallbacks[MetallicRoughness] = CreateTexture(tex);
        }
        // AO
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(white, white + 1), width, height, TextureType::Linear);
            RenderTexture linearFallback = CreateTexture(tex);
            fallbacks[AO] = linearFallback;
        }
        // Emissive
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(black, black + 4), width, height, TextureType::Srgb);
            fallbacks[Emissive] = CreateTexture(tex);
        }

        return fallbacks;
    }

    RenderTexture RenderMaterialData::CreateTexture(const Texture& sceneTex) const
    {
        const auto& pixels = sceneTex.GetPixels();
        const uint32_t width = sceneTex.GetWidth();
        const uint32_t height = sceneTex.GetHeight();
        const auto type = sceneTex.GetType();

        const VkFormat format = ToFormat(type);
        const uint32_t mipLevels = CalculateMipLevels(width, height);

        return Create2DTextureWithData(m_resources, pixels.data(), pixels.size(),
            width, height, format, mipLevels, m_resources.CreateSamplerLinearRepeatMip());
    }
}
