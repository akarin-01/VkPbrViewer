#include "scene.h"

#include "render/scene_proxy.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        void Scene::Update()
        {
            m_camera.Update();
            m_light.Update();
            m_skybox.Update();
            m_postProcess.Update();

            for (size_t i = 0; i < m_objects.size(); /*empty*/)
            {
                if (m_objects[i].IsDeletePending())
                {
                    Render::SceneProxy::Get().DeleteObject(m_objects[i].GetId());
                    std::swap(m_objects[i], m_objects.back());
                    m_objects.pop_back();
                    continue;
                }

                m_objects[i].Update();
                ++i;
            }
        }

        Object& Scene::CreateObject()
        {
            const Resource::ResourceId id = m_nextObjectId++;
            m_objects.emplace_back(id);
            return m_objects.back();
        }

        void Scene::DestroyObject(Resource::ResourceId id)
        {
            for (auto& object : m_objects)
            {
                if (object.GetId() == id)
                {
                    object.MarkDelete();
                    return;
                }
            }
        }
    }
}
