#pragma once

#include "resource/resource_id.h"
#include "scene/camera.h"
#include "scene/light.h"
#include "scene/object.h"
#include "scene/post_process.h"
#include "scene/skybox.h"

#include <vector>

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Scene
        {
        public:
            Scene() = default;
            ~Scene() = default;

            /// Writes the render-facing data (camera, light, object, skybox,
            /// post process) into the SceneProxy; called every frame
            void Update();

            Object& CreateObject();
            void DestroyObject(Resource::ResourceId id);

            const Camera& GetCamera() const { return m_camera; }
            Camera& GetCamera() { return m_camera; }

            const Light& GetLight() const { return m_light; }
            Light& GetLight() { return m_light; }

            const Skybox& GetSkybox() const { return m_skybox; }
            Skybox& GetSkybox() { return m_skybox; }

            const PostProcess& GetPostProcess() const { return m_postProcess; }
            PostProcess& GetPostProcess() { return m_postProcess; }

            const std::vector<Object>& GetObjects() const { return m_objects; }
            std::vector<Object>& GetObjects() { return m_objects; }

        private:
            Resource::ResourceId m_nextObjectId{ Resource::kInvalidId + 1 };

            Camera m_camera{};
            Light m_light{};
            Skybox m_skybox{};
            PostProcess m_postProcess{};
            std::vector<Object> m_objects{};
        };
    }
}
