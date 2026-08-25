#include "render_ibl_data.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_resources.h"
#include "render/compute_conversion.h"
#include "render/render_texture_set.h"

#include <cassert>

namespace Kita::Pbrv
{
    namespace
    {
        constexpr uint32_t kIrradianceSize = 32;
        constexpr uint32_t kPrefilterBaseSize = 128;
        constexpr uint32_t kBrdfLutSize = 512;
    }

    RenderIblData::RenderIblData(const RenderContext& context,
        RenderResources& resources,
        const DescriptorAllocator& descriptorAllocator)
        : m_context(context),
        m_resources(resources),
        m_descriptorAllocator(descriptorAllocator)
    {
        m_brdfConversion = std::make_unique<ComputeConversion>(m_context, m_resources, m_descriptorAllocator,
            0, "assets/shaders/brdf_integration_comp.spv", 0);
        m_irradianceConversion = std::make_unique<ComputeConversion>(m_context, m_resources, m_descriptorAllocator,
            1, "assets/shaders/irradiance_convolution_comp.spv", 0);
        m_prefilterConversion = std::make_unique<ComputeConversion>(m_context, m_resources, m_descriptorAllocator,
            1, "assets/shaders/prefilter_comp.spv", static_cast<uint32_t>(sizeof(PrefilterPC)));

        m_brdfLutSet = std::make_unique<BrdfLutSet>(m_context, m_resources, m_descriptorAllocator,
            BrdfLutArray{ CreateBrdfLut() });

        Log::Info("[Renderer] Create BRDF LUT: ", kBrdfLutSize, "x", kBrdfLutSize, " RG16F");

        m_textureSet = std::make_unique<TextureSet>(m_context, m_resources, m_descriptorAllocator,
            TextureArray{ CreateCubemapFallback(m_resources, m_context.HdrFormat()), CreateCubemapFallback(m_resources, m_context.HdrFormat()) });
    }

    RenderIblData::~RenderIblData() = default;

    void RenderIblData::Update(uint32_t frameIndex, const RenderTexture& sourceCubemap)
    {
        if (m_lastCubemap != sourceCubemap)
        {
            m_lastCubemap = sourceCubemap;

            // Source changed: convolve a new irradiance map, or restore the placeholder
            TextureArray updatedTexs{};

            if (sourceCubemap.IsEmpty())
            {
                updatedTexs = m_textureSet->GetFallbacks();
            }
            else
            {
                updatedTexs[0] = CreateIrradianceMap(sourceCubemap);
                updatedTexs[1] = CreatePrefilterEnvMap(sourceCubemap);

                Log::Info("[Renderer] Create irradiance map: ",
                    kIrradianceSize, "x", kIrradianceSize, "x6");
                Log::Info("[Renderer] Create prefilter env map: ",
                    kPrefilterBaseSize, "x", kPrefilterBaseSize, "x6");
            }

            m_textureSet->Update(updatedTexs);
            KITA_LOG_DEBUG("[Renderer] Update IBL descriptor set: textures changed");
        }

        m_textureSet->RefreshSet(frameIndex);
    }

    VkDescriptorSetLayout RenderIblData::GetSetLayout() const
    {
        return m_textureSet->GetLayout();
    }

    const VkDescriptorSet& RenderIblData::GetSet(uint32_t frameIndex) const
    {
        return m_textureSet->GetSet(frameIndex);
    }

    VkDescriptorSetLayout RenderIblData::GetBrdfLutSetLayout() const
    {
        return m_brdfLutSet->GetLayout();
    }

    const VkDescriptorSet& RenderIblData::GetBrdfLutSet(uint32_t frameIndex) const
    {
        return m_brdfLutSet->GetSet(frameIndex);
    }

    RenderTexture RenderIblData::CreateIrradianceMap(const RenderTexture& sourceCubemap) const
    {
        const VkFormat format = m_context.HdrFormat();

        // 1. Create irradiance map
        RenderTexture irradiance = CreateCubemapTexture(m_resources,
            kIrradianceSize, format, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            m_resources.CreateSamplerLinearClampNoMip());

        // 2. GPU conversion: dispatch irradiance_convolution compute shader
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 6;

        ComputeConversion::Output output{};
        output.m_image = irradiance.m_imageHandle;
        output.m_imageView = irradiance.m_imageViewHandle;
        output.m_range = range;

        ComputeConversion::Input input{};
        input.m_imageView = sourceCubemap.m_imageViewHandle;
        input.m_sampler = sourceCubemap.m_samplerHandle;

        m_irradianceConversion->Dispatch(output, { (kIrradianceSize + 7) / 8, (kIrradianceSize + 7) / 8, 6 },
            { input });

        return irradiance;
    }

    RenderTexture RenderIblData::CreatePrefilterEnvMap(const RenderTexture& sourceCubemap) const
    {
        const VkFormat format = m_context.HdrFormat();
        const uint32_t mipLevels = CalculateMipLevels(kPrefilterBaseSize, kPrefilterBaseSize);

        // 1. Create the prefiltered cubemap (all mips, UNDEFINED layout)
        RenderTexture prefilter = CreateCubemapTexture(m_resources,
            kPrefilterBaseSize, format, mipLevels,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            m_resources.CreateSamplerLinearClampMip());

        // 2. Convolve each mip separately: its own storage view and roughness
        for (uint32_t mip = 0; mip < mipLevels; ++mip)
        {
            const uint32_t mipSize = kPrefilterBaseSize >> mip;

            VkImageSubresourceRange mipRange{};
            mipRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            mipRange.baseMipLevel = mip;
            mipRange.levelCount = 1;
            mipRange.baseArrayLayer = 0;
            mipRange.layerCount = 6;

            RenderImageViewHandle mipView = m_resources.CreateImageView(
                prefilter.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE, mipRange);

            ComputeConversion::Output output{};
            output.m_image = prefilter.m_imageHandle;
            output.m_imageView = mipView;
            output.m_range = mipRange;

            PrefilterPC push{};
            push.m_roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);
            push.m_mipCount = static_cast<float>(mipLevels);

            ComputeConversion::Input input{};
            input.m_imageView = sourceCubemap.m_imageViewHandle;
            input.m_sampler = sourceCubemap.m_samplerHandle;

            m_prefilterConversion->Dispatch(output,
                { (mipSize + 7) / 8, (mipSize + 7) / 8, 6 }, { input }, &push);

            m_resources.DestroyImageView(mipView);   // one-shot view, dispatch is synchronous
        }

        return prefilter;
    }

    RenderTexture RenderIblData::CreateBrdfLut() const
    {
        // 1. Create the LUT texture (512x512 R16G16_SFLOAT, UNDEFINED layout)
        RenderTexture lut = Create2DTexture(m_resources,
            kBrdfLutSize, kBrdfLutSize, VK_FORMAT_R16G16_SFLOAT, 1,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            m_resources.CreateSamplerLinearClampNoMip());

        // 2. GPU integration: dispatch brdf_integration compute shader
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        ComputeConversion::Output output{};
        output.m_image = lut.m_imageHandle;
        output.m_imageView = lut.m_imageViewHandle;
        output.m_range = range;

        m_brdfConversion->Dispatch(output,
            { kBrdfLutSize / 16, kBrdfLutSize / 16, 1 }, {});

        return lut;
    }
}
