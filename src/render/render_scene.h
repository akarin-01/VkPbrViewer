#pragma once

#include "rhi/frame_info.h"
#include "resource/resource_types.h"
#include "resource/resource_id.h"
#include "render/render_target.h"
#include "render/data/render_frame_data.h"
#include "render/data/render_material_data.h"
#include "render/data/render_object_data.h"
#include "render/data/render_post_process_data.h"

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
        class SwapChain;
    }
    namespace Resource
    {
        class Resources;
        class DescriptorManager;
        class ResourceManager;
    }
    namespace Scene
    {
        class Scene;
        class Object;
    }

    namespace Render
    {
        struct RenderMeshData
        {
            Resource::MeshResource::Handle m_mesh{};
            Resource::ResourceId m_lastMeshId{ Resource::kInvalidId };
        };

        /// Owns the GPU-side render objects for the scene and syncs the CPU scene data each frame.
        /// Each render object manages its own lifetime; passes get their dependencies injected via GetXxx.
        class RenderScene
        {
        public:
            RenderScene(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                Resource::DescriptorManager& descriptorMgr,
                Resource::ResourceManager& resourceMgr);
            ~RenderScene();

            void Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo);
            void Recreate(VkExtent2D extent);

            const RenderFrameData& GetFrameData() const { return m_frameData; }
            const RenderMaterialData& GetMaterialData() const { return m_materialData; }
            const RenderObjectData& GetObjectData() const { return m_objectData; }
            const RenderMeshData& GetMeshData() const { return m_meshData; }
            const RenderPostProcessData& GetPostProcessData() const { return m_postProcessData; }

            const RenderTarget& GetTarget() const { return m_target; }
            RenderTarget& GetTarget() { return m_target; }

            VkDescriptorSetLayout GetEmptyLayout() const;

        private:
            void UpdateMesh(const Scene::Object& object);

        private:
            Resource::ResourceManager& m_resourceMgr;
            const Resource::DescriptorManager& m_descriptorMgr;

            RenderTarget m_target;

            RenderFrameData m_frameData;
            RenderMaterialData m_materialData;
            RenderObjectData m_objectData;
            RenderMeshData m_meshData;

            RenderPostProcessData m_postProcessData;
        };
    }
}
