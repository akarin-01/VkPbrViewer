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
        class ResourceManager;

        /// Bundle of compute conversions: equirect -> skybox cubemap,
        /// irradiance, prefilter and the BRDF integration LUT. Each conversion
        /// owns its pipeline; outputs are assembled through the ResourceManager.
        class EnvironmentBaker
        {
        public:
            EnvironmentBaker(const Rhi::Context& context,
                DescriptorManager& descriptorMgr,
                ResourceManager& resourceMgr);
            ~EnvironmentBaker();

            EnvironmentBaker(const EnvironmentBaker&) = delete;
            EnvironmentBaker& operator=(const EnvironmentBaker&) = delete;

            TextureResource BakeSkybox(const TextureResource& equirect);
            TextureResource BakeIrradiance(const TextureResource& skybox);
            TextureResource BakePrefilter(const TextureResource& skybox);
            TextureResource BakeBrdfLut();

        private:
            const Rhi::Context& m_context;
            DescriptorManager& m_descriptorMgr;
            ResourceManager& m_resourceMgr;

            std::unique_ptr<ComputeConversion> m_skyboxConversion;
            std::unique_ptr<ComputeConversion> m_irradianceConversion;
            std::unique_ptr<ComputeConversion> m_prefilterConversion;
            std::unique_ptr<ComputeConversion> m_brdfConversion;
        };
    }
}
