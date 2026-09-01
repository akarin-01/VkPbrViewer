#include "environment_baker.h"

#include "core/log.h"
#include "rhi/context.h"
#include "rhi/one_shot_command.h"
#include "rhi/utils.h"
#include "resource/compute_conversion.h"
#include "resource/constants.h"
#include "resource/descriptor_manager.h"
#include "resource/gpu_layouts.h"
#include "resource/resource_manager.h"
#include "resource/resource_utils.h"

#include <cmath>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace
        {
            SamplerDesc MakeClampSampler(SamplerDesc::MipMode mipMode)
            {
                SamplerDesc samplerDesc{};
                samplerDesc.m_magFilter = VK_FILTER_LINEAR;
                samplerDesc.m_minFilter = VK_FILTER_LINEAR;
                samplerDesc.m_mipMode = mipMode;
                samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                return samplerDesc;
            }

            ImageViewDesc MakeCubeViewDesc(bool fullRange, uint32_t baseMip = 0, uint32_t levelCount = 1)
            {
                ImageViewDesc viewDesc{};
                viewDesc.m_type = VK_IMAGE_VIEW_TYPE_CUBE;
                viewDesc.m_fullRange = fullRange;
                viewDesc.m_baseMipLevel = baseMip;
                viewDesc.m_levelCount = levelCount;
                viewDesc.m_layerCount = 6;
                return viewDesc;
            }
        }

        EnvironmentBaker::EnvironmentBaker(const Rhi::Context& context,
            DescriptorManager& descriptorMgr,
            ResourceManager& resourceMgr)
            : m_context(context),
            m_descriptorMgr(descriptorMgr),
            m_resourceMgr(resourceMgr)
        {
            m_skyboxConversion = std::make_unique<ComputeConversion>(m_context, m_descriptorMgr,
                DescriptorLayoutType::ComputeSample, "assets/shaders/equirect_to_cubemap_comp.spv", 0);
            m_irradianceConversion = std::make_unique<ComputeConversion>(m_context, m_descriptorMgr,
                DescriptorLayoutType::ComputeSample, "assets/shaders/irradiance_convolution_comp.spv",
                static_cast<uint32_t>(sizeof(Gpu::IrradiancePC)));
            m_prefilterConversion = std::make_unique<ComputeConversion>(m_context, m_descriptorMgr,
                DescriptorLayoutType::ComputeSample, "assets/shaders/prefilter_comp.spv",
                static_cast<uint32_t>(sizeof(Gpu::PrefilterPC)));
            m_brdfConversion = std::make_unique<ComputeConversion>(m_context, m_descriptorMgr,
                DescriptorLayoutType::ComputeWrite, "assets/shaders/brdf_integration_comp.spv", 0);
        }

        EnvironmentBaker::~EnvironmentBaker() = default;

        TextureResource EnvironmentBaker::BakeSkybox(const TextureResource& equirect)
        {
            // 1. Cubemap target: kCubemapFaceSize^2 x6, full mip chain
            const uint32_t mipLevels = ResourceUtils::CalculateMipLevels(kCubemapFaceSize, kCubemapFaceSize);
            ImageDesc cubeDesc{};
            cubeDesc.m_extent = { kCubemapFaceSize, kCubemapFaceSize, 1 };
            cubeDesc.m_mipLevels = mipLevels;
            cubeDesc.m_arrayLayers = 6;
            cubeDesc.m_format = m_context.HdrFormat();
            cubeDesc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            cubeDesc.m_usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            cubeDesc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            cubeDesc.m_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            ImageRhi::Handle cubeImage = m_resourceMgr.CreateImage(cubeDesc);

            // One-shot storage view: base mip, all 6 layers
            ImageViewRhi::Handle storageView =
                m_resourceMgr.CreateImageView(MakeCubeViewDesc(false), cubeImage);

            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 6;

            // 2. GPU conversion: equirect -> cubemap base mip
            ComputeConversion::Output output{};
            output.m_image = cubeImage->m_image;
            output.m_imageView = storageView->m_imageView;
            output.m_range = range;
            output.m_finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            output.m_finalStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            output.m_finalAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT;

            ComputeConversion::Input input{};
            input.m_imageView = equirect.GetImageView();
            input.m_sampler = equirect.GetSampler();

            m_skyboxConversion->Dispatch(output,
                { (kCubemapFaceSize + 7) / 8, (kCubemapFaceSize + 7) / 8, 6 }, { input });

            // 3. Generate the remaining mips (1..N)
            {
                VkImageSubresourceRange mipRange{};
                mipRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipRange.baseMipLevel = 1;
                mipRange.levelCount = mipLevels - 1;
                mipRange.baseArrayLayer = 0;
                mipRange.layerCount = 6;

                Rhi::OneShotCommand command(m_context);
                Rhi::TransitionImageLayout(command.Handle(), cubeImage->m_image,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    mipRange);
                ResourceUtils::GenerateImageMipmaps(command.Handle(), *cubeImage);
            }

            // 4. Assemble the sampler-facing cubemap texture
            Core::Log::Info("[Resource] Create skybox cubemap: ", kCubemapFaceSize, "x",
                kCubemapFaceSize, "x6, ", mipLevels, " mips");
            return m_resourceMgr.CreateTexture(cubeImage,
                MakeCubeViewDesc(true), MakeClampSampler(SamplerDesc::MipMode::Linear));
        }

        TextureResource EnvironmentBaker::BakeIrradiance(const TextureResource& skybox)
        {
            ImageDesc desc{};
            desc.m_extent = { kIrradianceSize, kIrradianceSize, 1 };
            desc.m_arrayLayers = 6;
            desc.m_format = m_context.HdrFormat();
            desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            desc.m_usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            desc.m_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            ImageRhi::Handle image = m_resourceMgr.CreateImage(desc);

            // One-shot storage view: base mip, all 6 layers
            ImageViewRhi::Handle storageView =
                m_resourceMgr.CreateImageView(MakeCubeViewDesc(false), image);

            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 6;

            ComputeConversion::Output output{};
            output.m_image = image->m_image;
            output.m_imageView = storageView->m_imageView;
            output.m_range = range;

            // Sample the source from the mip matching the irradiance resolution
            // (low-pass filter kills the sun-peak variance in the convolution)
            ComputeConversion::Input input{};
            input.m_imageView = skybox.GetImageView();
            input.m_sampler = skybox.GetSampler();

            Gpu::IrradiancePC push{};
            push.m_envMip = static_cast<float>(
                std::log2(static_cast<double>(kCubemapFaceSize) / kIrradianceSize));

            m_irradianceConversion->Dispatch(output,
                { (kIrradianceSize + 7) / 8, (kIrradianceSize + 7) / 8, 6 }, { input }, &push);

            Core::Log::Info("[Resource] Create irradiance map: ", kIrradianceSize, "x",
                kIrradianceSize, "x6");
            return m_resourceMgr.CreateTexture(image,
                MakeCubeViewDesc(true), MakeClampSampler(SamplerDesc::MipMode::None));
        }

        TextureResource EnvironmentBaker::BakePrefilter(const TextureResource& skybox)
        {
            const uint32_t mipLevels = ResourceUtils::CalculateMipLevels(kPrefilterBaseSize, kPrefilterBaseSize);

            ImageDesc desc{};
            desc.m_extent = { kPrefilterBaseSize, kPrefilterBaseSize, 1 };
            desc.m_mipLevels = mipLevels;
            desc.m_arrayLayers = 6;
            desc.m_format = m_context.HdrFormat();
            desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            desc.m_usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            desc.m_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            ImageRhi::Handle image = m_resourceMgr.CreateImage(desc);

            ComputeConversion::Input input{};
            input.m_imageView = skybox.GetImageView();
            input.m_sampler = skybox.GetSampler();

            // Convolve each mip separately: its own storage view and roughness
            for (uint32_t mip = 0; mip < mipLevels; ++mip)
            {
                const uint32_t mipSize = kPrefilterBaseSize >> mip;

                ImageViewRhi::Handle mipView =
                    m_resourceMgr.CreateImageView(MakeCubeViewDesc(false, mip), image);

                VkImageSubresourceRange mipRange{};
                mipRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipRange.baseMipLevel = mip;
                mipRange.levelCount = 1;
                mipRange.baseArrayLayer = 0;
                mipRange.layerCount = 6;

                ComputeConversion::Output output{};
                output.m_image = image->m_image;
                output.m_imageView = mipView->m_imageView;
                output.m_range = mipRange;

                Gpu::PrefilterPC push{};
                push.m_roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);
                push.m_mipCount = static_cast<float>(mipLevels);

                m_prefilterConversion->Dispatch(output,
                    { (mipSize + 7) / 8, (mipSize + 7) / 8, 6 }, { input }, &push);
            }

            Core::Log::Info("[Resource] Create prefilter env map: ", kPrefilterBaseSize, "x",
                kPrefilterBaseSize, "x6, ", mipLevels, " mips");
            return m_resourceMgr.CreateTexture(image,
                MakeCubeViewDesc(true), MakeClampSampler(SamplerDesc::MipMode::Linear));
        }

        TextureResource EnvironmentBaker::BakeBrdfLut()
        {
            ImageDesc desc{};
            desc.m_extent = { kBrdfLutSize, kBrdfLutSize, 1 };
            desc.m_format = VK_FORMAT_R16G16_SFLOAT;
            desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            desc.m_usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            ImageRhi::Handle image = m_resourceMgr.CreateImage(desc);

            ImageViewDesc viewDesc{};
            viewDesc.m_type = VK_IMAGE_VIEW_TYPE_2D;
            viewDesc.m_fullRange = true;
            ImageViewRhi::Handle view = m_resourceMgr.CreateImageView(viewDesc, image);
            SamplerRhi::Handle sampler =
                m_resourceMgr.GetOrCreateSampler(MakeClampSampler(SamplerDesc::MipMode::None));

            TextureResource lut{ image, view, sampler };

            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            ComputeConversion::Output output{};
            output.m_image = image->m_image;
            output.m_imageView = view->m_imageView;
            output.m_range = range;

            m_brdfConversion->Dispatch(output,
                { kBrdfLutSize / 16, kBrdfLutSize / 16, 1 }, {});

            return lut;
        }
    }
}
