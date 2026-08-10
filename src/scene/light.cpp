#include "light.h"

namespace Kita::Pbrv
{
    Light::Light() = default;

    Light::~Light() = default;

    Light& Light::SetDirection(const glm::vec3& dir)
    {
        m_direction = glm::normalize(dir);
        return *this;
    }

    Light& Light::SetColor(const glm::vec3& color)
    {
        m_color = color;
        return *this;
    }

    Light& Light::SetIntensity(float intensity)
    {
        m_intensity = intensity;
        return *this;
    }
}