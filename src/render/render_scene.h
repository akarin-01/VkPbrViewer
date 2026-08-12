#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include "scene/texture.h"
#include "scene/mesh.h"

#include <vulkan/vulkan.h>
#include <array>
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
        void CreateSamplers();
        void DestroySamplers();
        void CreateFallbackTextures();
        void DestroyFallbackTextures();

        RenderPerFrame CreateRenderPerFrame();
        void DestroyRenderPerFrame(RenderPerFrame& frame);
        RenderMaterial CreateRenderMaterial();
        void DestroyRenderMaterial(RenderMaterial& material);
        RenderMesh CreateRenderMesh(const Mesh& sceneMesh);
        void DestroyRenderMesh(RenderMesh& mesh);
        RenderTexture CreateRenderTexture(const Texture& sceneTex, RenderSamplerHandle samplerHandle);
        void DestroyRenderTexture(RenderTexture& texture);

    private:
        RenderResources& m_resources;
        const SwapChain& m_swapChain;

        RenderList m_list{};

        RenderSamplerHandle m_linearRepeatSamplerHandle{ 0 };
        std::array<RenderTexture, kMaterialTextureCount> m_fallbackTextures{ 0 };
    };
}