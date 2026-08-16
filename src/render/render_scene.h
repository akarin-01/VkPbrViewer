#pragma once

#include "render/frame_data.h"
#include "render/material_cache.h"
#include "render/mesh_cache.h"
#include "render/skybox_environment.h"

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

        const FrameData& GetFrameData() const { return m_frameData; }
        const MaterialCache& GetMaterialCache() const { return m_materialCache; }
        const MeshCache& GetMeshCache() const { return m_meshCache; }
        const SkyboxEnvironment& GetSkyboxEnvironment() const { return m_skyboxEnvironment; }

    private:
        FrameData m_frameData;
        MaterialCache m_materialCache;
        MeshCache m_meshCache;
        SkyboxEnvironment m_skyboxEnvironment;
    };
}
