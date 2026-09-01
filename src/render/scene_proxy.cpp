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

        SceneProxy::ObjectInput& SceneProxy::FindOrAddObjectInput(Resource::ResourceId id)
        {
            for (auto& objectInput : m_objectInputs)
            {
                if (objectInput.m_id == id)
                {
                    return objectInput;
                }
            }

            return m_objectInputs.emplace_back(ObjectInput{ id });
        }
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

        void SceneProxy::WriteObjectData(Resource::ResourceId id, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
        {
            auto& objectInput = FindOrAddObjectInput(id);
            objectInput.m_position = position;
            objectInput.m_rotation = rotation;
            objectInput.m_scale = scale;
        }

        void SceneProxy::WriteObjectMaterial(Resource::ResourceId id,
            const glm::vec4& albedo, float metallic, float roughness, float ao,
            const glm::vec3& emissive, float emissiveIntensity)
        {
            auto& objectInput = FindOrAddObjectInput(id);
            objectInput.m_albedo = albedo;
            objectInput.m_metallic = metallic;
            objectInput.m_roughness = roughness;
            objectInput.m_ao = ao;
            objectInput.m_emissive = emissive;
            objectInput.m_emissiveIntensity = emissiveIntensity;
        }

        void SceneProxy::WritePostProcessData(float ev)
        {
            m_postProcessInput.m_ev = ev;
        }

        void SceneProxy::UpdateEnvironment(Resource::ResourceId equirectId)
        {
            m_environment = EnvironmentRecord{ equirectId };
        }

        void SceneProxy::UpdateMesh(Resource::ResourceId id, Resource::ResourceId meshId)
        {
            m_meshRecords.emplace_back(MeshRecord{ id, meshId });
        }

        void SceneProxy::UpdateMaterial(Resource::ResourceId id, const std::array<Resource::ResourceId, Resource::kMaterialSlotCount>& textureIds)
        {
            m_materialRecords.emplace_back(MaterialRecord{ id, textureIds });
        }

        void SceneProxy::DeleteObject(Resource::ResourceId id)
        {
            m_deletedObjects.push_back(id);
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
            m_objectDatas.resize(m_objectInputs.size());
            for (size_t i = 0; i < m_objectInputs.size(); ++i)
            {
                auto& objectData = m_objectDatas[i];
                auto& object = objectData.m_object;
                auto& objectInput = m_objectInputs[i];

                objectData.m_id = objectInput.m_id;

                object.m_transform.m_model = CalculateModelMatrix(
                    objectInput.m_position, objectInput.m_rotation, objectInput.m_scale);
                object.m_transform.m_normal = CalculateNormalMatrix(object.m_transform.m_model);
                object.m_material.m_albedo = objectInput.m_albedo;
                object.m_material.m_pbrParams = glm::vec4(
                    objectInput.m_metallic, objectInput.m_roughness, objectInput.m_ao, 0.0f);
                object.m_material.m_emissive =
                    glm::vec4(objectInput.m_emissive, objectInput.m_emissiveIntensity);
            }

            // Raw inputs are fully converted: drop them for the next frame
            m_cameraInput = {};
            m_lightInput = {};
            m_objectInputs.clear();
            m_postProcessInput = {};
        }

        void SceneProxy::Reset()
        {
            // GPU outputs and records were consumed: clear them
            m_frameData = {};
            m_postProcessData = {};
            m_objectDatas.clear();
            m_environment = {};
            m_meshRecords.clear();
            m_materialRecords.clear();
            m_deletedObjects.clear();
        }
    }
}
