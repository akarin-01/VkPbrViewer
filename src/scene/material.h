#pragma once

#include "resource/constants.h"
#include "resource/asset_types.h"

#include <glm/glm.hpp>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Material
        {
        public:
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

            void SetAlbedoTex(Resource::TextureAsset::Handle texture) { m_albedoTex = texture; }
            Resource::TextureAsset::Handle GetAlbedoTex() const { return m_albedoTex; }
            void SetNormalTex(Resource::TextureAsset::Handle texture) { m_normalTex = texture; }
            Resource::TextureAsset::Handle GetNormalTex() const { return m_normalTex; }
            void SetMRTex(Resource::TextureAsset::Handle texture) { m_mrTex = texture; }
            Resource::TextureAsset::Handle GetMRTex() const { return m_mrTex; }
            void SetAOTex(Resource::TextureAsset::Handle texture) { m_aoTex = texture; }
            Resource::TextureAsset::Handle GetAOTex() const { return m_aoTex; }
            void SetEmissiveTex(Resource::TextureAsset::Handle texture) { m_emissiveTex = texture; }
            Resource::TextureAsset::Handle GetEmissiveTex() const { return m_emissiveTex; }

            Resource::TextureAsset::Handle GetTexture(uint32_t slot) const
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

            void SetTexture(uint32_t slot, Resource::TextureAsset::Handle texture)
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

            Resource::TextureAsset::Handle m_albedoTex;
            Resource::TextureAsset::Handle m_normalTex;
            Resource::TextureAsset::Handle m_mrTex;
            Resource::TextureAsset::Handle m_aoTex;
            Resource::TextureAsset::Handle m_emissiveTex;
        };
    }
}
