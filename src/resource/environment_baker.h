#pragma once

#include "resource/resource_types.h"

#include <memory>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        class ComputeConversion;
        class DescriptorManager;

        /// Raw bake output: an untracked image plus the view/sampler assembly
        /// descs; the ResourceManager registers it into its tables.
        struct BakedTexture
        {
            ImageRhi m_image{};
            ImageViewDesc m_viewDesc{};
            SamplerDesc m_samplerDesc{};
        };

        /// Bundle of compute conversions: equirect -> skybox cubemap,
        /// irradiance, prefilter and the BRDF integration LUT. Each conversion
        /// owns its pipeline; outputs are returned raw for the ResourceManager
        /// to assemble.
        class EnvironmentBaker
        {
        public:
            EnvironmentBaker(const Rhi::Context& context, DescriptorManager& descriptorMgr);
            ~EnvironmentBaker();

            EnvironmentBaker(const EnvironmentBaker&) = delete;
            EnvironmentBaker& operator=(const EnvironmentBaker&) = delete;

            BakedTexture BakeSkybox(const TextureResource& equirect);
            BakedTexture BakeIrradiance(const TextureResource& skybox);
            BakedTexture BakePrefilter(const TextureResource& skybox);
            BakedTexture BakeBrdfLut();

        private:
            const Rhi::Context& m_context;
            DescriptorManager& m_descriptorMgr;

            std::unique_ptr<ComputeConversion> m_skyboxConversion;
            std::unique_ptr<ComputeConversion> m_irradianceConversion;
            std::unique_ptr<ComputeConversion> m_prefilterConversion;
            std::unique_ptr<ComputeConversion> m_brdfConversion;
        };
    }
}
