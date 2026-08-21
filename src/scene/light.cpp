#include "light.h"

namespace Kita::Pbrv
{
    Light::Light() = default;

    Light::~Light() = default;

    Light& Light::SetPosition(const glm::vec3& pos)
    {
        m_position = pos;
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
