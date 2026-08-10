#pragma once

#include <glm/glm.hpp>

namespace Kita::Pbrv
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

        glm::vec4 GetAlbedo() const { return m_albedo; }
        float GetMetallic() const { return m_metallic; }
        float GetRoughness() const { return m_roughness; }
        float GetAO() const { return m_ao; }

    private:
        glm::vec4 m_albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
        float m_metallic{ 0.0f };
        float m_roughness{ 1.0f };
        float m_ao{ 1.0f };
    };
}