#pragma once

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/object.h"
#include "scene/post_process.h"
#include "scene/skybox.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        /// Only supports one model for now.
        class Scene
        {
        public:
            Scene() = default;
            ~Scene() = default;

            const Camera& GetCamera() const { return m_camera; }
            Camera& GetCamera() { return m_camera; }

            const Light& GetLight() const { return m_light; }
            Light& GetLight() { return m_light; }

            const Object& GetObject() const { return m_object; }
            Object& GetObject() { return m_object; }

            const Skybox& GetSkybox() const { return m_skybox; }
            Skybox& GetSkybox() { return m_skybox; }

            const PostProcess& GetPostProcess() const { return m_postProcess; }
            PostProcess& GetPostProcess() { return m_postProcess; }

        private:
            Camera m_camera{};
            Light m_light{};
            Object m_object{};
            Skybox m_skybox{};
            PostProcess m_postProcess{};
        };
    }
}
