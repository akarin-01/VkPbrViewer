#include "material.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Scene
    {
        Material::Material() = default;

        Material::~Material() = default;

        Material& Material::SetAlbedo(const glm::vec4& albedo)
        {
            m_albedo = albedo;
            return *this;
        }

        Material& Material::SetMetallic(float metallic)
        {
            m_metallic = metallic;
            return *this;
        }

        Material& Material::SetRoughness(float roughness)
        {
            m_roughness = roughness;
            return *this;
        }

        Material& Material::SetAO(float ao)
        {
            m_ao = ao;
            return *this;
        }

        Material& Material::SetEmissive(glm::vec3 emissive)
        {
            m_emissive = emissive;
            return *this;
        }

        Material& Material::SetEmissiveIntensity(float intensity)
        {
            m_emissiveIntensity = intensity;
            return *this;
        }

        Material& Material::SetAlbedoTex(Resource::TextureAsset::Handle texture)
        {
            if (texture.GetId() == m_albedoTex.GetId())
            {
                return *this;   // duplicate set
            }
            m_albedoTex = std::move(texture);
            m_texturesDirty = true;
            return *this;
        }

        Material& Material::SetNormalTex(Resource::TextureAsset::Handle texture)
        {
            if (texture.GetId() == m_normalTex.GetId())
            {
                return *this;   // duplicate set
            }
            m_normalTex = std::move(texture);
            m_texturesDirty = true;
            return *this;
        }

        Material& Material::SetMRTex(Resource::TextureAsset::Handle texture)
        {
            if (texture.GetId() == m_mrTex.GetId())
            {
                return *this;   // duplicate set
            }
            m_mrTex = std::move(texture);
            m_texturesDirty = true;
            return *this;
        }

        Material& Material::SetAOTex(Resource::TextureAsset::Handle texture)
        {
            if (texture.GetId() == m_aoTex.GetId())
            {
                return *this;   // duplicate set
            }
            m_aoTex = std::move(texture);
            m_texturesDirty = true;
            return *this;
        }

        Material& Material::SetEmissiveTex(Resource::TextureAsset::Handle texture)
        {
            if (texture.GetId() == m_emissiveTex.GetId())
            {
                return *this;   // duplicate set
            }
            m_emissiveTex = std::move(texture);
            m_texturesDirty = true;
            return *this;
        }

        Material& Material::SetTexture(Resource::MaterialSlot slot, Resource::TextureAsset::Handle texture)
        {
            switch (slot)
            {
            case Resource::MaterialSlot::Albedo:            SetAlbedoTex(std::move(texture)); break;
            case Resource::MaterialSlot::Normal:            SetNormalTex(std::move(texture)); break;
            case Resource::MaterialSlot::MetallicRoughness: SetMRTex(std::move(texture)); break;
            case Resource::MaterialSlot::AO:                SetAOTex(std::move(texture)); break;
            case Resource::MaterialSlot::Emissive:          SetEmissiveTex(std::move(texture)); break;
            default:
                throw std::runtime_error("Invalid texture slot!");
            }
            return *this;
        }
    }
}
