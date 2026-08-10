#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

namespace Kita::Pbrv
{
    class RenderResources;
    class SwapChain;
    class Scene;

    struct Vertex;

    class RenderScene
    {
    public:
        RenderScene(RenderResources& resources, const SwapChain& swapChain);
        ~RenderScene();

        void Update(const Scene& scene, const FrameInfo& frameInfo);
        RenderList GetRenderList() const;

    private:
        RenderPerFrame CreateRenderPerFrame();
        void DestroyRenderPerFrame(RenderPerFrame& frame);
        RenderMaterial CreateRenderMaterial();
        void DestroyRenderMaterial(RenderMaterial& material);
        RenderMesh CreateRenderMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        void DestroyRenderMesh(RenderMesh& mesh);

    private:
        RenderResources& m_resources;
        const SwapChain& m_swapChain;

        RenderList m_list;
    };
}