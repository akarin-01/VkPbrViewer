#include "material.h"

namespace Kita::Pbrv
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
}