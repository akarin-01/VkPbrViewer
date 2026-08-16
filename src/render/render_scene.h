#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include "scene/scene.h"

#include <vulkan/vulkan.h>
#include <array>
#include <vector>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class SwapChain;
    class DescriptorAllocator;
    class Scene;

    struct Vertex;

    class RenderScene
    {
    public:
        RenderScene(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderScene();

        void Update(const Scene& scene, const FrameInfo& frameInfo);
        const RenderList& GetRenderList() const;

    private:
        void CreateFallbackTextures();
        void DestroyFallbackTextures();

        RenderPerFrame CreateRenderPerFrame() const;
        void DestroyRenderPerFrame(RenderPerFrame& frame);
        void UpdateRenderPerFrame(RenderPerFrame& frame, uint32_t frameIndex, const Camera& sceneCamera, const Light& sceneLight) const;

        RenderMaterial CreateRenderMaterial() const;
        void DestroyRenderMaterial(RenderMaterial& material);
        void UpdateRenderMaterial(RenderMaterial& material, uint32_t frameIndex, const Material& sceneMat);
        void WriteMaterialSet(VkDescriptorSet set, const std::array<RenderTexture, kMaterialTextureCount>& textures) const;

        RenderMesh CreateRenderMesh(const Mesh& sceneMesh) const;
        void DestroyRenderMesh(RenderMesh& mesh) const;
        bool UpdateRenderMesh(RenderMesh& mesh, const Mesh& sceneMesh) const;

        RenderTexture CreateRenderTexture(const Texture& sceneTex) const;
        void DestroyRenderTexture(RenderTexture& texture) const;
        bool UpdateRenderTexture(RenderTexture& texture, uint32_t slot, const Texture& sceneTex) const;

        RenderSkybox CreateRenderSkybox(const Skybox& sceneSkybox) const;
        void DestroyRenderSkybox(RenderSkybox& skybox) const;
        bool UpdateRenderSkybox(RenderSkybox& skybox, uint32_t frameIndex, const Skybox& sceneSkybox) const;
        void WriteSkyboxSet(VkDescriptorSet set, const RenderTexture& texture) const;

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const SwapChain& m_swapChain;
        const DescriptorAllocator& m_descriptorAllocator;

        RenderList m_list{};

        std::array<RenderTexture, kMaterialTextureCount> m_fallbackTextures{ 0 };
    };
}