#include "render_material_data.h"
#include "core/log.h"
#include "rhi/context.h"
#include "resource/resources.h"
#include "resource/texture_set.h"
#include "scene/material.h"
#include "scene/texture.h"

#include <glm/glm.hpp>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            VkFormat ToFormat(Scene::TextureType type)
            {
                switch (type)
                {
                case Scene::TextureType::Srgb:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case Scene::TextureType::Normal:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case Scene::TextureType::MetallicRoughness:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case Scene::TextureType::Linear:
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

            const Scene::Texture& GetTexture(const Scene::Material& mat, uint32_t slot)
            {
                switch (slot)
                {
                case Resource::Albedo:
                    return mat.GetAlbedoTex();
                case Resource::Normal:
                    return mat.GetNormalTex();
                case Resource::MetallicRoughness:
                    return mat.GetMRTex();
                case Resource::AO:
                    return mat.GetAOTex();
                case Resource::Emissive:
                    return mat.GetEmissiveTex();
                default:
                    throw std::runtime_error("Invalid texture slot!");
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

                        Core::Log::Info("[Renderer] Create texture [", ToString(Resource::MaterialTextureSlot(i)), "]: ",
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

            // Resource::Albedo
            {
                Scene::Texture tex{};
                tex.SetData("fallback", std::vector<uint8_t>(white, white + 4), width, height, Scene::TextureType::Srgb);
                fallbacks[Resource::Albedo] = CreateTexture(tex);
            }
            // Resource::Normal
            {
                Scene::Texture tex{};
                tex.SetData("fallback", std::vector<uint8_t>(flat, flat + 4), width, height, Scene::TextureType::Normal);
                fallbacks[Resource::Normal] = CreateTexture(tex);
            }
            // MR
            {
                Scene::Texture tex{};
                tex.SetData("fallback", std::vector<uint8_t>(white, white + 4), width, height, Scene::TextureType::MetallicRoughness);
                fallbacks[Resource::MetallicRoughness] = CreateTexture(tex);
            }
            // Resource::AO
            {
                Scene::Texture tex{};
                tex.SetData("fallback", std::vector<uint8_t>(white, white + 1), width, height, Scene::TextureType::Linear);
                Resource::RenderTexture linearFallback = CreateTexture(tex);
                fallbacks[Resource::AO] = linearFallback;
            }
            // Resource::Emissive
            {
                Scene::Texture tex{};
                tex.SetData("fallback", std::vector<uint8_t>(black, black + 4), width, height, Scene::TextureType::Srgb);
                fallbacks[Resource::Emissive] = CreateTexture(tex);
            }

            return fallbacks;
        }

        Resource::RenderTexture RenderMaterialData::CreateTexture(const Scene::Texture& sceneTex) const
        {
            const auto& pixels = sceneTex.GetPixels();
            const uint32_t width = sceneTex.GetWidth();
            const uint32_t height = sceneTex.GetHeight();
            const auto type = sceneTex.GetType();

            const VkFormat format = ToFormat(type);
            const uint32_t mipLevels = Rhi::CalculateMipLevels(width, height);

            return Resource::Create2DTextureWithData(m_resources, pixels.data(), pixels.size(),
                width, height, format, mipLevels, m_resources.CreateSamplerLinearRepeatMip());
        }
    }
}
