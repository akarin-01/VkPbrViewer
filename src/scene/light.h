#pragma once

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    class Light
    {
    public:
        Light();
        ~Light();

        Light& SetPosition(const glm::vec3& pos);
        Light& SetColor(const glm::vec3& color);
        Light& SetIntensity(float intensity);

        glm::vec3 GetPosition() const { return m_position; }
        glm::vec3 GetColor() const { return m_color; }
        float GetIntensity() const { return m_intensity; }

    private:
        glm::vec3 m_position{ 1.0f, 1.0f, 0.0f };
        glm::vec3 m_color{ 1.0f, 1.0f, 1.0f };
        float m_intensity{ 1.0f };
    };
}
