#include "scene_proxy.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            glm::mat4 CalculateViewMatrix(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up)
            {
                return glm::lookAt(position, position + front, up);
            }

            glm::mat4 CalculateProjectionMatrix(float fov, float aspect, float near, float far)
            {
                glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, near, far);
                proj[1][1] *= -1.0f;   // Vulkan: flip Y to match the framebuffer
                return proj;
            }

            glm::mat4 CalculateModelMatrix(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
            {
                glm::mat4 model{ 1.0f };
                model = glm::translate(model, position);
                model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, scale);
                return model;
            }

            glm::mat3 CalculateNormalMatrix(const glm::mat4& model)
            {
                return glm::transpose(glm::inverse(glm::mat3(model)));
            }
        }

        SceneProxy& SceneProxy::Get()
        {
            static SceneProxy instance{};
            return instance;
        }

        SceneProxy::SceneProxy() = default;
        SceneProxy::~SceneProxy() = default;

        void SceneProxy::WriteCameraData(const glm::vec3& position, const glm::vec3& front,
            const glm::vec3& up, float fov, float near, float far)
        {
            m_cameraInput.m_position = position;
            m_cameraInput.m_front = front;
            m_cameraInput.m_up = up;
            m_cameraInput.m_fov = fov;
            m_cameraInput.m_near = near;
            m_cameraInput.m_far = far;
        }

        void SceneProxy::WriteLightData(const glm::vec3& position, const glm::vec3& color, float intensity)
        {
            m_lightInput.m_position = position;
            m_lightInput.m_color = color;
            m_lightInput.m_intensity = intensity;
        }

        void SceneProxy::WriteObjectData(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
        {
            m_objectInput.m_position = position;
            m_objectInput.m_rotation = rotation;
            m_objectInput.m_scale = scale;
        }

        void SceneProxy::WriteObjectMaterial(const glm::vec4& albedo,
            float metallic, float roughness, float ao, const glm::vec3& emissive,
            float emissiveIntensity)
        {
            m_objectInput.m_albedo = albedo;
            m_objectInput.m_metallic = metallic;
            m_objectInput.m_roughness = roughness;
            m_objectInput.m_ao = ao;
            m_objectInput.m_emissive = emissive;
            m_objectInput.m_emissiveIntensity = emissiveIntensity;
        }

        void SceneProxy::WritePostProcessData(float ev)
        {
            m_postProcessInput.m_ev = ev;
        }

        void SceneProxy::UpdateEnvironment(Resource::ResourceId equirectId)
        {
            m_environment = EnvironmentRecord{ equirectId };
        }

        void SceneProxy::UpdateMesh(Resource::ResourceId meshId)
        {
            m_mesh = MeshRecord{ meshId };
        }

        void SceneProxy::UpdateMaterial(const std::array<Resource::ResourceId, Resource::kMaterialSlotCount>& textureIds)
        {
            m_material = MaterialRecord{ textureIds };
        }

        void SceneProxy::BuildSceneProxy(float aspect)
        {
            // Camera + light -> per frame
            const glm::mat4 view = CalculateViewMatrix(
                m_cameraInput.m_position, m_cameraInput.m_front, m_cameraInput.m_up);
            const glm::mat4 proj = CalculateProjectionMatrix(
                m_cameraInput.m_fov, aspect, m_cameraInput.m_near, m_cameraInput.m_far);

            m_frameData.m_camera.m_position = glm::vec4(m_cameraInput.m_position, 1.0f);
            m_frameData.m_camera.m_viewProj = proj * view;
            m_frameData.m_camera.m_skyboxViewProj = proj * glm::mat4(glm::mat3(view));
            m_frameData.m_light.m_position = glm::vec4(m_lightInput.m_position, 0.0f);
            m_frameData.m_light.m_colorIntensity =
                glm::vec4(m_lightInput.m_color, m_lightInput.m_intensity);

            // Post process
            m_postProcessData.m_exposure =
                glm::vec4(std::exp2(m_postProcessInput.m_ev), 0.0f, 0.0f, 0.0f);

            // Object
            m_objectData.m_transform.m_model = CalculateModelMatrix(
                m_objectInput.m_position, m_objectInput.m_rotation, m_objectInput.m_scale);
            m_objectData.m_transform.m_normal = CalculateNormalMatrix(m_objectData.m_transform.m_model);
            m_objectData.m_material.m_albedo = m_objectInput.m_albedo;
            m_objectData.m_material.m_pbrParams = glm::vec4(
                m_objectInput.m_metallic, m_objectInput.m_roughness, m_objectInput.m_ao, 0.0f);
            m_objectData.m_material.m_emissive =
                glm::vec4(m_objectInput.m_emissive, m_objectInput.m_emissiveIntensity);

            // Raw inputs are fully converted: drop them for the next frame
            m_cameraInput = {};
            m_lightInput = {};
            m_objectInput = {};
            m_postProcessInput = {};
        }

        void SceneProxy::Reset()
        {
            // GPU outputs and records were consumed: clear them
            m_frameData = {};
            m_postProcessData = {};
            m_objectData = {};
            m_environment = {};
            m_mesh = {};
            m_material = {};
        }
    }
}
