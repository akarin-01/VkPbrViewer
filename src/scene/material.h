#pragma once

#include "resource/constants.h"
#include "resource/texture.h"

#include <glm/glm.hpp>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Material
        {
        public:
            using TextureHandle = Resource::Texture::Handle;

            Material();
            ~Material();

            Material& SetAlbedo(const glm::vec4& albedo);
            Material& SetMetallic(float metallic);
            Material& SetRoughness(float roughness);
            Material& SetAO(float ao);
            Material& SetEmissive(glm::vec3 emissive);

            glm::vec4 GetAlbedo() const { return m_albedo; }
            float GetMetallic() const { return m_metallic; }
            float GetRoughness() const { return m_roughness; }
            float GetAO() const { return m_ao; }
            glm::vec3 GetEmissive() const { return m_emissive; }

            void SetAlbedoTex(TextureHandle texture) { m_albedoTex = texture; }
            TextureHandle GetAlbedoTex() const { return m_albedoTex; }
            void SetNormalTex(TextureHandle texture) { m_normalTex = texture; }
            TextureHandle GetNormalTex() const { return m_normalTex; }
            void SetMRTex(TextureHandle texture) { m_mrTex = texture; }
            TextureHandle GetMRTex() const { return m_mrTex; }
            void SetAOTex(TextureHandle texture) { m_aoTex = texture; }
            TextureHandle GetAOTex() const { return m_aoTex; }
            void SetEmissiveTex(TextureHandle texture) { m_emissiveTex = texture; }
            TextureHandle GetEmissiveTex() const { return m_emissiveTex; }

            TextureHandle GetTexture(uint32_t slot) const
            {
                switch (slot)
                {
                case Resource::MaterialTextureSlot::Albedo:
                    return m_albedoTex;
                case Resource::MaterialTextureSlot::Normal:
                    return m_normalTex;
                case Resource::MaterialTextureSlot::MetallicRoughness:
                    return m_mrTex;
                case Resource::MaterialTextureSlot::AO:
                    return m_aoTex;
                case Resource::MaterialTextureSlot::Emissive:
                    return m_emissiveTex;
                default:
                    throw std::runtime_error("Invalid texture slot!");
                }
            }

            void SetTexture(uint32_t slot, TextureHandle texture)
            {
                switch (slot)
                {
                case Resource::MaterialTextureSlot::Albedo:
                    m_albedoTex = std::move(texture);
                    break;
                case Resource::MaterialTextureSlot::Normal:
                    m_normalTex = std::move(texture);
                    break;
                case Resource::MaterialTextureSlot::MetallicRoughness:
                    m_mrTex = std::move(texture);
                    break;
                case Resource::MaterialTextureSlot::AO:
                    m_aoTex = std::move(texture);
                    break;
                case Resource::MaterialTextureSlot::Emissive:
                    m_emissiveTex = std::move(texture);
                    break;
                default:
                    throw std::runtime_error("Invalid texture slot!");
                }
            }

        private:
            glm::vec4 m_albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
            float m_metallic{ 1.0f };
            float m_roughness{ 1.0f };
            float m_ao{ 1.0f };
            glm::vec3 m_emissive{ 1.0f, 1.0f, 1.0f };

            TextureHandle m_albedoTex;
            TextureHandle m_normalTex;
            TextureHandle m_mrTex;
            TextureHandle m_aoTex;
            TextureHandle m_emissiveTex;
        };
    }
}
