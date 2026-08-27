#pragma once

#include "rhi/frame_info.h"
#include "render/data/render_frame_data.h"
#include "render/data/render_ibl_data.h"
#include "render/data/render_material_data.h"
#include "render/data/render_mesh_data.h"
#include "render/data/render_post_process_data.h"
#include "render/data/render_skybox_data.h"

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
    }
    namespace Scene
    {
        class Scene;
    }

    /// Owns the GPU-side render objects for the scene and syncs the CPU scene data each frame.
    /// Each render object manages its own lifetime; passes get their dependencies injected via GetXxx.
    namespace Render
    {
        class RenderScene
        {
        public:
            RenderScene(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                Resource::DescriptorManager& descriptorMgr);
            ~RenderScene();

            void Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo);

            const RenderFrameData& GetFrameData() const { return m_frameData; }
            const RenderMaterialData& GetMaterialData() const { return m_materialData; }
            const RenderMeshData& GetMeshData() const { return m_meshData; }
            const RenderSkyboxData& GetSkyboxData() const { return m_skyboxData; }
            const RenderPostProcessData& GetPostProcessData() const { return m_postProcessData; }
            const RenderIblData& GetIblData() const { return m_iblData; }

        private:
            RenderFrameData m_frameData;
            RenderMaterialData m_materialData;
            RenderMeshData m_meshData;
            RenderSkyboxData m_skyboxData;
            RenderPostProcessData m_postProcessData;
            RenderIblData m_iblData;
        };
    }
}
