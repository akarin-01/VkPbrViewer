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

            Material& SetParams(const Resource::MaterialParams& params);

            Material& SetAlbedoTex(Resource::TextureView::Handle texture);
            Material& SetNormalTex(Resource::TextureView::Handle texture);
            Material& SetMRTex(Resource::TextureView::Handle texture);
            Material& SetAOTex(Resource::TextureView::Handle texture);
            Material& SetEmissiveTex(Resource::TextureView::Handle texture);
            Material& SetTexture(Resource::MaterialSlot slot, Resource::TextureView::Handle texture);

            glm::vec4 GetAlbedo() const { return m_params.m_baseColorFactor; }
            float GetMetallic() const { return m_params.m_metallicFactor; }
            float GetRoughness() const { return m_params.m_roughnessFactor; }
            float GetAO() const { return m_params.m_ao; }
            glm::vec3 GetEmissive() const { return m_params.m_emissiveFactor; }
            float GetEmissiveIntensity() const { return m_params.m_emissiveIntensity; }

            const Resource::MaterialParams& GetParams() const { return m_params; }
            Resource::MaterialParams& GetParams() { return m_params; }

            Resource::TextureView::Handle GetAlbedoTex() const { return m_albedoTex; }
            Resource::TextureView::Handle GetNormalTex() const { return m_normalTex; }
            Resource::TextureView::Handle GetMRTex() const { return m_mrTex; }
            Resource::TextureView::Handle GetAOTex() const { return m_aoTex; }
            Resource::TextureView::Handle GetEmissiveTex() const { return m_emissiveTex; }

            Resource::TextureView::Handle GetTexture(Resource::MaterialSlot slot) const
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

            /// Returns true when any texture view changed since the last consume
            bool ConsumeTexturesDirty()
            {
                const bool dirty = m_texturesDirty;
                m_texturesDirty = false;
                return dirty;
            }

        private:
            Resource::MaterialParams m_params{
                glm::vec4(1.0f),
                1.0f,
                1.0f,
                1.0f,
                glm::vec3(1.0f),
                1.0f
            };

            Resource::TextureView::Handle m_albedoTex;
            Resource::TextureView::Handle m_normalTex;
            Resource::TextureView::Handle m_mrTex;
            Resource::TextureView::Handle m_aoTex;
            Resource::TextureView::Handle m_emissiveTex;

            bool m_texturesDirty{ true };
        };
    }
}
