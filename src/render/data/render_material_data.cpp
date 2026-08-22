#include "render_material_data.h"

#include "core/log.h"
#include "scene/material.h"
#include "scene/texture.h"
#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/descriptor_writer.h"

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
        CreateFallbacks();

        // Textures
        m_textures = m_fallbackTextures;

        // Set layout
        {
            std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[0].descriptorCount = kMaterialTextureCount;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_setLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Sets
        {
            for (auto& set : m_sets)
            {
                set = m_descriptorAllocator.Allocate(m_setLayout, "Material set");
                WriteSet(set);
            }
        }
    }

    RenderMaterialData::~RenderMaterialData()
    {
        // Sets will be destroyed automatically

        // Textures (skip fallbacks; they are destroyed together below)
        for (size_t i = 0; i < m_textures.size(); ++i)
        {
            if (m_textures[i] != m_fallbackTextures[i])
            {
                DestroyTexture(m_textures[i]);
            }
        }

        // Set layout
        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);

        DestroyFallbacks();
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
        for (uint32_t i = 0; i < kMaterialTextureCount; ++i)
        {
            anyTexUpdated |= UpdateTextureSlot(i, GetTexture(sceneMat, i));
        }

        if (anyTexUpdated)
        {
            m_setRefreshCount = kMaxFramesInFlight;

            KITA_LOG_DEBUG("[Renderer] Update material descriptor set: textures changed");
        }

        bool setRefreshed = m_setRefreshCount > 0;
        if (setRefreshed)
        {
            WriteSet(m_sets[frameIndex]);
            --m_setRefreshCount;
        }
    }

    void RenderMaterialData::CreateFallbacks()
    {
        uint8_t white[] = { 255, 255, 255, 255 };
        uint8_t black[] = { 0, 0, 0, 255 };
        uint8_t flat[] = { 128, 128, 255, 255 };
        uint32_t width = 1;
        uint32_t height = 1;

        // Albedo
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(white, white + 4), width, height, TextureType::Srgb);
            m_fallbackTextures[Albedo] = CreateTexture(tex);
        }
        // Normal
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(flat, flat + 4), width, height, TextureType::Normal);
            m_fallbackTextures[Normal] = CreateTexture(tex);
        }
        // MR
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(white, white + 4), width, height, TextureType::MetallicRoughness);
            m_fallbackTextures[MetallicRoughness] = CreateTexture(tex);
        }
        // AO
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(white, white + 1), width, height, TextureType::Linear);
            RenderTexture linearFallback = CreateTexture(tex);
            m_fallbackTextures[AO] = linearFallback;
        }
        // Emissive
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(black, black + 4), width, height, TextureType::Srgb);
            m_fallbackTextures[Emissive] = CreateTexture(tex);
        }
    }

    void RenderMaterialData::DestroyFallbacks()
    {
        for (auto& texture : m_fallbackTextures)
        {
            DestroyTexture(texture);
        }
    }

    bool RenderMaterialData::UpdateTextureSlot(uint32_t slot, const Texture& sceneTex)
    {
        if (sceneTex.GetRevision() != m_lastSyncedRevisions[slot])
        {
            m_lastSyncedRevisions[slot] = sceneTex.GetRevision();

            bool isOldFallback = m_textures[slot] == m_fallbackTextures[slot];
            if (!isOldFallback)
            {
                DestroyTexture(m_textures[slot]);
            }

            RenderTexture newTex{};
            if (sceneTex.IsEmpty())
            {
                // New texture is empty, use fallback
                newTex = m_fallbackTextures[slot];
            }
            else
            {
                // New texture is not empty, create new texture
                newTex = CreateTexture(sceneTex);

                Log::Info("[Renderer] Upload ", ToString(MaterialTextureSlot(slot)), " texture: ",
                    sceneTex.GetName(), ", ", sceneTex.GetPixelCount(), " bytes");
            }
            m_textures[slot] = newTex;

            return true;
        }

        return false;
    }

    RenderTexture RenderMaterialData::CreateTexture(const Texture& sceneTex) const
    {
        auto& pixels = sceneTex.GetPixels();
        auto width = sceneTex.GetWidth();
        auto height = sceneTex.GetHeight();
        auto type = sceneTex.GetType();

        VkFormat format = ToFormat(type);
        auto mipLevels = CalculateMipLevels(width, height);

        RenderTexture texture{};

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        texture.m_imageHandle = m_resources.CreateImageWithData(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            pixels.data(), pixels.size());
        m_resources.GenerateImageMipmaps(texture.m_imageHandle);

        texture.m_imageViewHandle = m_resources.CreateImageView(texture.m_imageHandle, VK_IMAGE_VIEW_TYPE_2D);

        texture.m_samplerHandle = m_resources.CreateSamplerLinearRepeatMip();

        return texture;
    }

    void RenderMaterialData::DestroyTexture(RenderTexture& texture) const
    {
        m_resources.DestroySampler(texture.m_samplerHandle);
        m_resources.DestroyImageView(texture.m_imageViewHandle);
        m_resources.DestroyImage(texture.m_imageHandle);

        texture = {};
    }

    void RenderMaterialData::WriteSet(VkDescriptorSet set)
    {
        DescriptorWriter writer(m_resources, m_context.Device());
        for (uint32_t i = 0; i < kMaterialTextureCount; ++i)
        {
            auto& texture = m_textures[i];

            writer.WriteImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.m_imageViewHandle, texture.m_samplerHandle);
        }

        writer.UpdateSet(set);
    }
}
