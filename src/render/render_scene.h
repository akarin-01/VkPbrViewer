#pragma once

#include "render/data/render_frame_data.h"
#include "render/data/render_material_data.h"
#include "render/data/render_mesh_data.h"
#include "render/data/render_skybox_data.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class SwapChain;
    class DescriptorAllocator;
    class Scene;

    /// Owns the GPU-side render objects for the scene and syncs the CPU scene data each frame.
    /// Each render object manages its own lifetime; passes get their dependencies injected via GetXxx.
    class RenderScene
    {
    public:
        RenderScene(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderScene();

        void Update(const Scene& scene, const FrameInfo& frameInfo);

        const RenderFrameData& GetRenderFrameData() const { return m_frameData; }
        const RenderMaterialData& GetRenderMaterialData() const { return m_materialData; }
        const RenderMeshData& GetRenderMeshData() const { return m_meshData; }
        const RenderSkyboxData& GetRenderSkyboxData() const { return m_skyboxData; }

    private:
        RenderFrameData m_frameData;
        RenderMaterialData m_materialData;
        RenderMeshData m_meshData;
        RenderSkyboxData m_skyboxData;
    };
}
