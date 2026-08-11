#pragma once

#include "scene/texture.h"

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
        Texture& GetAlbedoTex() { return m_albedoTex; }
        const Texture& GetAlbedoTex() const { return m_albedoTex; }
        Texture& GetNormalTex() { return m_normalTex; }
        const Texture& GetNormalTex() const { return m_normalTex; }
        Texture& GetMetallicTex() { return m_metallicTex; }
        const Texture& GetMetallicTex() const { return m_metallicTex; }
        Texture& GetRoughnessTex() { return m_roughnessTex; }
        const Texture& GetRoughnessTex() const { return m_roughnessTex; }
        Texture& GetAOTex() { return m_aoTex; }
        const Texture& GetAOTex() const { return m_aoTex; }

    private:
        glm::vec4 m_albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
        float m_metallic{ 1.0f };
        float m_roughness{ 1.0f };
        float m_ao{ 1.0f };

        Texture m_albedoTex;
        Texture m_normalTex;
        Texture m_metallicTex;
        Texture m_roughnessTex;
        Texture m_aoTex;
    };
}