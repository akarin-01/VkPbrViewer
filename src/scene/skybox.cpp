#include "skybox.h"

#include "render/scene_proxy.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        void Skybox::Update()
        {
            if (m_skyboxDirty)
            {
                m_skyboxDirty = false;
                Render::SceneProxy::Get().UpdateEnvironment(m_skybox.GetId());
            }
        }

        Skybox& Skybox::SetSkybox(Resource::TextureAsset::Handle skybox)
        {
            if (skybox.GetId() == m_skybox.GetId())
            {
                return *this;   // duplicate set
            }
            m_skybox = std::move(skybox);
            m_skyboxDirty = true;
            return *this;
        }
    }
}
