#include "render_ibl_data.h"
#include "core/log.h"
#include "rhi/context.h"
#include "rhi/utils.h"
#include "resource/compute_conversion.h"
#include "resource/descriptor_manager.h"
#include "resource/resources.h"
#include "resource/texture_set.h"

#include <cmath>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr Resource::DescriptorLayoutType kBrdfLayoutType = Resource::DescriptorLayoutType::BrdfLut;
            constexpr Resource::DescriptorLayoutType kTextureLayoutType = Resource::DescriptorLayoutType::IblTex;
            constexpr Resource::DescriptorLayoutType kBrdfConversionLayoutType = Resource::DescriptorLayoutType::ComputeWrite;
            constexpr Resource::DescriptorLayoutType kSampleConversionLayoutType = Resource::DescriptorLayoutType::ComputeSample;

            // Push constants, only used by the conversion dispatches below
            struct PrefilterPC
            {
                float m_roughness{ 0.0f };  // 0..1, selects the mip level
                float m_mipCount{ 1.0f };
            };

            struct IrradiancePC
            {
                float m_envMip{ 0.0f };    // source cubemap sampling lod, computed from face sizes
            };
        }

        RenderIblData::RenderIblData(const Rhi::Context& context,
            Resource::Resources& resources,
            Resource::DescriptorManager& descriptorMgr)
            : m_context(context),
            m_resources(resources),
            m_descriptorMgr(descriptorMgr)
        {
            m_brdfConversion = std::make_unique<Resource::ComputeConversion>(m_context, m_resources, m_descriptorMgr,
                kBrdfConversionLayoutType, "assets/shaders/brdf_integration_comp.spv", 0);
            m_irradianceConversion = std::make_unique<Resource::ComputeConversion>(m_context, m_resources, m_descriptorMgr,
                kSampleConversionLayoutType, "assets/shaders/irradiance_convolution_comp.spv", static_cast<uint32_t>(sizeof(IrradiancePC)));
            m_prefilterConversion = std::make_unique<Resource::ComputeConversion>(m_context, m_resources, m_descriptorMgr,
                kSampleConversionLayoutType, "assets/shaders/prefilter_comp.spv", static_cast<uint32_t>(sizeof(PrefilterPC)));

            m_brdfLutSet = std::make_unique<BrdfLutSet>(m_context, m_resources,
                m_descriptorMgr, kBrdfLayoutType,
                BrdfLutArray{ CreateBrdfLut() });

            Core::Log::Info("[Renderer] Create BRDF LUT: ", Resource::kBrdfLutSize, "x", Resource::kBrdfLutSize, " RG16F");

            m_iblTextureSet = std::make_unique<IblTextureSet>(m_context, m_resources,
                m_descriptorMgr, kTextureLayoutType,
                IblTextureArray{ Resource::CreateCubemapFallback(m_resources, m_context.HdrFormat()), Resource::CreateCubemapFallback(m_resources, m_context.HdrFormat()) });
        }

        RenderIblData::~RenderIblData() = default;

        void RenderIblData::Update(uint32_t frameIndex, const Resource::RenderTexture& sourceCubemap)
        {
            if (m_lastCubemap != sourceCubemap)
            {
                m_lastCubemap = sourceCubemap;

                // Source changed: convolve a new irradiance map, or restore the placeholder
                IblTextureArray updatedTexs{};

                if (sourceCubemap.IsEmpty())
                {
                    updatedTexs = m_iblTextureSet->GetFallbacks();
                }
                else
                {
                    updatedTexs[0] = CreateIrradianceMap(sourceCubemap);
                    updatedTexs[1] = CreatePrefilterEnvMap(sourceCubemap);

                    Core::Log::Info("[Renderer] Create irradiance map: ",
                        Resource::kIrradianceSize, "x", Resource::kIrradianceSize, "x6");
                    Core::Log::Info("[Renderer] Create prefilter env map: ",
                        Resource::kPrefilterBaseSize, "x", Resource::kPrefilterBaseSize, "x6");
                }

                m_iblTextureSet->Update(updatedTexs);
                KITA_LOG_DEBUG("[Renderer] Update IBL descriptor set: textures changed");
            }

            m_iblTextureSet->RefreshSet(frameIndex);
        }

        VkDescriptorSetLayout RenderIblData::GetSetLayout() const
        {
            return m_iblTextureSet->GetLayout();
        }

        const VkDescriptorSet& RenderIblData::GetSet(uint32_t frameIndex) const
        {
            return m_iblTextureSet->GetSet(frameIndex);
        }

        VkDescriptorSetLayout RenderIblData::GetBrdfLutSetLayout() const
        {
            return m_brdfLutSet->GetLayout();
        }

        const VkDescriptorSet& RenderIblData::GetBrdfLutSet(uint32_t frameIndex) const
        {
            return m_brdfLutSet->GetSet(frameIndex);
        }

        Resource::RenderTexture RenderIblData::CreateIrradianceMap(const Resource::RenderTexture& sourceCubemap) const
        {
            const VkFormat format = m_context.HdrFormat();

            // 1. Create irradiance map
            Resource::RenderTexture irradiance = Resource::CreateCubemapTexture(m_resources,
                Resource::kIrradianceSize, format, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_resources.CreateSamplerLinearClampNoMip());

            // 2. GPU conversion: dispatch irradiance_convolution compute shader
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 6;

            Resource::ComputeConversion::Output output{};
            output.m_image = irradiance.m_imageHandle;
            output.m_imageView = irradiance.m_imageViewHandle;
            output.m_range = range;

            Resource::ComputeConversion::Input input{};
            input.m_imageView = sourceCubemap.m_imageViewHandle;
            input.m_sampler = sourceCubemap.m_samplerHandle;

            // Sample the source from the mip matching the irradiance resolution
            // (low-pass filter kills the sun-peak variance in the convolution)
            IrradiancePC push{};
            push.m_envMip = static_cast<float>(
                std::log2(static_cast<double>(Resource::kCubemapFaceSize) / Resource::kIrradianceSize));

            m_irradianceConversion->Dispatch(output, { (Resource::kIrradianceSize + 7) / 8, (Resource::kIrradianceSize + 7) / 8, 6 },
                { input }, &push);

            return irradiance;
        }

        Resource::RenderTexture RenderIblData::CreatePrefilterEnvMap(const Resource::RenderTexture& sourceCubemap) const
        {
            const VkFormat format = m_context.HdrFormat();
            const uint32_t mipLevels = Rhi::CalculateMipLevels(Resource::kPrefilterBaseSize, Resource::kPrefilterBaseSize);

            // 1. Create the prefiltered cubemap (all mips, UNDEFINED layout)
            Resource::RenderTexture prefilter = Resource::CreateCubemapTexture(m_resources,
                Resource::kPrefilterBaseSize, format, mipLevels,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_resources.CreateSamplerLinearClampMip());

            // 2. Convolve each mip separately: its own storage view and roughness
            for (uint32_t mip = 0; mip < mipLevels; ++mip)
            {
                const uint32_t mipSize = Resource::kPrefilterBaseSize >> mip;

                VkImageSubresourceRange mipRange{};
                mipRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipRange.baseMipLevel = mip;
                mipRange.levelCount = 1;
                mipRange.baseArrayLayer = 0;
                mipRange.layerCount = 6;

                Resource::RenderImageViewHandle mipView = m_resources.CreateImageView(
                    prefilter.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE, mipRange);

                Resource::ComputeConversion::Output output{};
                output.m_image = prefilter.m_imageHandle;
                output.m_imageView = mipView;
                output.m_range = mipRange;

                PrefilterPC push{};
                push.m_roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);
                push.m_mipCount = static_cast<float>(mipLevels);

                Resource::ComputeConversion::Input input{};
                input.m_imageView = sourceCubemap.m_imageViewHandle;
                input.m_sampler = sourceCubemap.m_samplerHandle;

                m_prefilterConversion->Dispatch(output,
                    { (mipSize + 7) / 8, (mipSize + 7) / 8, 6 }, { input }, &push);

                m_resources.DestroyImageView(mipView);   // one-shot view, dispatch is synchronous
            }

            return prefilter;
        }

        Resource::RenderTexture RenderIblData::CreateBrdfLut() const
        {
            // 1. Create the LUT texture (512x512 R16G16_SFLOAT, UNDEFINED layout)
            Resource::RenderTexture lut = Resource::Create2DTexture(m_resources,
                Resource::kBrdfLutSize, Resource::kBrdfLutSize, VK_FORMAT_R16G16_SFLOAT, 1,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_resources.CreateSamplerLinearClampNoMip());

            // 2. GPU integration: dispatch brdf_integration compute shader
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            Resource::ComputeConversion::Output output{};
            output.m_image = lut.m_imageHandle;
            output.m_imageView = lut.m_imageViewHandle;
            output.m_range = range;

            m_brdfConversion->Dispatch(output,
                { Resource::kBrdfLutSize / 16, Resource::kBrdfLutSize / 16, 1 }, {});

            return lut;
        }
    }
}
