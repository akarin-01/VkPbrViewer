#pragma once

#include "resource/constants.h"
#include "resource/gpu_layouts.h"
#include "resource/resource_id.h"

#include <array>
#include <glm/glm.hpp>
#include <optional>

namespace Kita::Pbrv
{
    namespace Render
    {
        /// An environment change: set when the skybox asset id differs
        struct EnvironmentRecord
        {
            Resource::ResourceId m_equirectId{ Resource::kInvalidId };
        };

        /// A mesh change: set when the object's mesh handle differs
        struct MeshRecord
        {
            Resource::ResourceId m_id{ Resource::kInvalidId };
            Resource::ResourceId m_meshId{ Resource::kInvalidId };   // kInvalidId = remove mesh
        };

        /// A material texture change: full id array, set on any slot change
        struct MaterialRecord
        {
            Resource::ResourceId m_id{ Resource::kInvalidId };
            std::array<Resource::ResourceId, Resource::kMaterialSlotCount> m_textureIds{};
        };

        struct ObjectData
        {
            Resource::ResourceId m_id{ Resource::kInvalidId };
            Resource::Gpu::PerObject m_object{};
        };

        /// Local static singleton. Scene entities write raw inputs and change
        /// records; BuildSceneProxy converts the inputs into GPU data, which
        /// RenderScene consumes together with the records, then Reset() clears
        class SceneProxy
        {
        public:
            static SceneProxy& Get();

            ~SceneProxy();

            // ---- Scene writes: raw per-frame inputs (converted by Build) ----
            void WriteCameraData(const glm::vec3& position, const glm::vec3& front,
                const glm::vec3& up, float fov, float near, float far);
            void WriteLightData(const glm::vec3& position, const glm::vec3& color, float intensity);
            void WriteObjectData(Resource::ResourceId id,
                const glm::vec3& position,
                const glm::vec3& rotation = glm::vec3(0.0f),    // euler degrees
                const glm::vec3& scale = glm::vec3(1.0f));
            void WriteObjectMaterial(Resource::ResourceId id,
                const glm::vec4& albedo, float metallic, float roughness, float ao,
                const glm::vec3& emissive, float emissiveIntensity);
            void WritePostProcessData(float ev);

            // ---- Scene writes: change records (only called when a value differs) ----
            void UpdateEnvironment(Resource::ResourceId equirectId);
            void UpdateMesh(Resource::ResourceId id, Resource::ResourceId meshId);
            void UpdateMaterial(Resource::ResourceId id,
                const std::array<Resource::ResourceId, Resource::kMaterialSlotCount>& textureIds);
            void DeleteObject(Resource::ResourceId id);

            // ---- Render layer reads ----
            void BuildSceneProxy(float aspect);
            void Reset();

            const Resource::Gpu::PerFrame& GetFrameData() const { return m_frameData; }
            const Resource::Gpu::PostProcess& GetPostProcessData() const { return m_postProcessData; }
            const std::vector<ObjectData>& GetObjectDatas() const { return m_objectDatas; }
            const std::optional<EnvironmentRecord>& GetEnvironmentRecord() const { return m_environment; }
            const std::vector<MeshRecord>& GetMeshRecords() const { return m_meshRecords; }
            const std::vector<MaterialRecord>& GetMaterialRecords() const { return m_materialRecords; }
            const std::vector<Resource::ResourceId>& GetDeletedObjects() const { return m_deletedObjects; }

        private:
            struct CameraInput
            {
                glm::vec3 m_position{ 0.0f };
                glm::vec3 m_front{ 0.0f, 0.0f, -1.0f };
                glm::vec3 m_up{ 0.0f, 1.0f, 0.0f };
                float m_fov{ 0.0f };
                float m_near{ 0.0f };
                float m_far{ 0.0f };
            };

            struct LightInput
            {
                glm::vec3 m_position{ 0.0f };
                glm::vec3 m_color{ 1.0f };
                float m_intensity{ 0.0f };
            };

            struct ObjectInput
            {
                Resource::ResourceId m_id{ Resource::kInvalidId };

                glm::vec3 m_position{ 0.0f };
                glm::vec3 m_rotation{ 0.0f };
                glm::vec3 m_scale{ 1.0f };
                glm::vec4 m_albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
                float m_metallic{ 0.0f };
                float m_roughness{ 0.0f };
                float m_ao{ 0.0f };
                glm::vec3 m_emissive{ 0.0f };
                float m_emissiveIntensity{ 0.0f };
            };

            struct PostProcessInput
            {
                float m_ev{ 0.0f };
            };

            SceneProxy();

            ObjectInput& FindOrAddObjectInput(Resource::ResourceId id);

        private:

            // ------------------ Scene input ------------------------

            CameraInput m_cameraInput{};
            LightInput m_lightInput{};
            PostProcessInput m_postProcessInput{};
            std::vector<ObjectInput> m_objectInputs{};

            // ------------------ Render output -----------------------

            Resource::Gpu::PerFrame m_frameData{};
            Resource::Gpu::PostProcess m_postProcessData{};
            std::vector<ObjectData> m_objectDatas{};

            std::optional<EnvironmentRecord> m_environment{};
            std::vector<MeshRecord> m_meshRecords{};
            std::vector<MaterialRecord> m_materialRecords{};
            std::vector<Resource::ResourceId> m_deletedObjects{};
        };
    }
}
