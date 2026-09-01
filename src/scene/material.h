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
            Material& SetEmissiveIntensity(float intensity);

            Material& SetAlbedoTex(Resource::TextureAsset::Handle texture);
            Material& SetNormalTex(Resource::TextureAsset::Handle texture);
            Material& SetMRTex(Resource::TextureAsset::Handle texture);
            Material& SetAOTex(Resource::TextureAsset::Handle texture);
            Material& SetEmissiveTex(Resource::TextureAsset::Handle texture);
            Material& SetTexture(Resource::MaterialSlot slot, Resource::TextureAsset::Handle texture);

            glm::vec4 GetAlbedo() const { return m_albedo; }
            float GetMetallic() const { return m_metallic; }
            float GetRoughness() const { return m_roughness; }
            float GetAO() const { return m_ao; }
            glm::vec3 GetEmissive() const { return m_emissive; }
            float GetEmissiveIntensity() const { return m_emissiveIntensity; }

            Resource::TextureAsset::Handle GetAlbedoTex() const { return m_albedoTex; }
            Resource::TextureAsset::Handle GetNormalTex() const { return m_normalTex; }
            Resource::TextureAsset::Handle GetMRTex() const { return m_mrTex; }
            Resource::TextureAsset::Handle GetAOTex() const { return m_aoTex; }
            Resource::TextureAsset::Handle GetEmissiveTex() const { return m_emissiveTex; }

            Resource::TextureAsset::Handle GetTexture(Resource::MaterialSlot slot) const
            {
                switch (slot)
                {
                case Resource::MaterialSlot::Albedo:            return GetAlbedoTex();
                case Resource::MaterialSlot::Normal:            return GetNormalTex();
                case Resource::MaterialSlot::MetallicRoughness: return GetMRTex();
                case Resource::MaterialSlot::AO:                return GetAOTex();
                case Resource::MaterialSlot::Emissive:          return GetEmissiveTex();
                default:
                    throw std::runtime_error("Invalid texture slot!");
                }
            }

            /// Returns true when any texture handle changed since the last consume
            bool ConsumeTexturesDirty()
            {
                const bool dirty = m_texturesDirty;
                m_texturesDirty = false;
                return dirty;
            }

        private:
            glm::vec4 m_albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
            float m_metallic{ 1.0f };
            float m_roughness{ 1.0f };
            float m_ao{ 1.0f };
            glm::vec3 m_emissive{ 1.0f, 1.0f, 1.0f };
            float m_emissiveIntensity{ 1.0f };

            Resource::TextureAsset::Handle m_albedoTex;
            Resource::TextureAsset::Handle m_normalTex;
            Resource::TextureAsset::Handle m_mrTex;
            Resource::TextureAsset::Handle m_aoTex;
            Resource::TextureAsset::Handle m_emissiveTex;

            bool m_texturesDirty{ true };
        };
    }
}
