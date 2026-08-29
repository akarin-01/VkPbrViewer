#pragma once

#include "core/macro.h"
#include "rhi/constants.h"
#include "resource/types.h"
#include "resource/render_texture.h"
#include "resource/resource_id.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <memory>

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Camera;
        class Light;
        class Skybox;
    }
    namespace Rhi
    {
        class Context;
        class SwapChain;
    }
    namespace Resource
    {
        class Resources;
        class DescriptorManager;
        class ComputeConversion;
        struct TextureAsset;
    }

    namespace Render
    {
        struct PerFrame
        {
            struct Camera
            {
                alignas(16) glm::mat4 m_viewProj{ 1.0f };
                alignas(16) glm::mat4 m_skyboxViewProj{ {1.0f} };
                alignas(16) glm::vec4 m_position{ 0.0f, 0.0f, 0.0f, 1.0f };         // xyz - pos, w - 1 always
            };

            struct Light
            {
                alignas(16) glm::vec4 m_position{ 0.0f, 0.0f, 0.0f, 0.0f };         // xyz - pos, w - 0(directional light)
                alignas(16) glm::vec4 m_colorIntensity{ 0.0f };                     // xyz - rgb, w - intensity
            };

            Camera m_camera{};
            Light m_light{};
        };
        STD140_ASSERT(PerFrame, 176);

        class RenderFrameData
        {
        public:
            RenderFrameData(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                Resource::DescriptorManager& descriptorMgr);
            ~RenderFrameData();

            void UpdateUbo(uint32_t frameIndex, const Scene::Camera& camera, const Scene::Light& light);
            void UpdateSkybox(const Scene::Skybox& sceneSkybox);
            void RefreshSet(uint32_t frameIndex);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

        private:
            Resource::RenderTexture CreateBrdfLut() const;
            Resource::RenderTexture CreateSkybox(const Resource::TextureAsset& texture) const;
            Resource::RenderTexture CreateIrradianceMap(const Resource::RenderTexture& sourceCubemap) const;
            Resource::RenderTexture CreatePrefilterEnvMap(const Resource::RenderTexture& sourceCubemap) const;
            void DestroyTextureSafe(Resource::RenderTexture& texture) const;

            void WriteSet(uint32_t frameIndex, bool writeUbo = false) const;

            Resource::RenderTexture CreateCubemapFallback() const;
            Resource::RenderTexture CreateEquirectTexture(const Resource::TextureAsset& texture, VkFormat format) const;

        private:
            const Rhi::Context& m_context;
            Resource::Resources& m_resources;
            const Rhi::SwapChain& m_swapChain;
            Resource::DescriptorManager& m_descriptorMgr;

            std::unique_ptr<Resource::ComputeConversion> m_brdfConversion;
            std::unique_ptr<Resource::ComputeConversion> m_skyboxConversion;
            std::unique_ptr<Resource::ComputeConversion> m_irradianceConversion;
            std::unique_ptr<Resource::ComputeConversion> m_prefilterConversion;

            std::array<Resource::RenderBufferHandle, Rhi::kMaxFramesInFlight> m_uboHandles{};
            Resource::RenderTexture m_brdfLut{};
            Resource::RenderTexture m_skybox{};
            Resource::RenderTexture m_irradiance{};
            Resource::RenderTexture m_prefilter{};
            Resource::RenderTexture m_cubemapFallback{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};

            Resource::ResourceId m_lastSkyboxId{ Resource::kInvalidId };
            uint32_t m_setDirtyCount{ 0 };
        };
    }
}
