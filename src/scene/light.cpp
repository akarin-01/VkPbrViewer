#include "light.h"

#include "render/scene_proxy.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        Light::Light() = default;

        Light::~Light() = default;

        void Light::Update() const
        {
            Render::SceneProxy::Get().WriteLightData(m_position, m_color, m_intensity);
        }

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
}
