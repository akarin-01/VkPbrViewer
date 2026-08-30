#include "render_frame_data.h"

#include "core/log.h"
#include "rhi/context.h"
#include "rhi/utils.h"
#include "rhi/descriptor_writer.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_manager.h"
#include "resource/gpu_layouts.h"
#include "resource/resources.h"
#include "resource/compute_conversion.h"
#include "resource/constants.h"
#include "scene/camera.h"
#include "scene/light.h"
#include "scene/skybox.h"

#include <glm/glm.hpp>
#include <cmath>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr Resource::DescriptorLayoutType kLayoutType = Resource::DescriptorLayoutType::PerFrame;
            constexpr Resource::DescriptorLayoutType kComputeWriteLayoutType = Resource::DescriptorLayoutType::ComputeWrite;
            constexpr Resource::DescriptorLayoutType kComputeSampleLayoutType = Resource::DescriptorLayoutType::ComputeSample;
        }

        RenderFrameData::RenderFrameData(const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            Resource::DescriptorManager& descriptorMgr)
            : m_context(context),
            m_resources(resources),
            m_swapChain(swapChain),
            m_descriptorMgr(descriptorMgr)
        {
            // Conversions
            {
                m_brdfConversion = std::make_unique<Resource::ComputeConversion>(m_context, m_resources, m_descriptorMgr,
                    kComputeWriteLayoutType, "assets/shaders/brdf_integration_comp.spv", 0);
                m_skyboxConversion = std::make_unique<Resource::ComputeConversion>(m_context, m_resources, m_descriptorMgr,
                    kComputeSampleLayoutType, "assets/shaders/equirect_to_cubemap_comp.spv", 0);
                m_irradianceConversion = std::make_unique<Resource::ComputeConversion>(m_context, m_resources, m_descriptorMgr,
                    kComputeSampleLayoutType, "assets/shaders/irradiance_convolution_comp.spv", static_cast<uint32_t>(sizeof(Gpu::IrradiancePC)));
                m_prefilterConversion = std::make_unique<Resource::ComputeConversion>(m_context, m_resources, m_descriptorMgr,
                    kComputeSampleLayoutType, "assets/shaders/prefilter_comp.spv", static_cast<uint32_t>(sizeof(Gpu::PrefilterPC)));
            }

            // UBO
            {
                VkBufferCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                createInfo.size = sizeof(Gpu::PerFrame);
                createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                for (auto& handle : m_uboHandles)
                {
                    handle = m_resources.CreateBuffer(createInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
                }
            }

            // Textures
            {
                m_cubemapFallback = CreateCubemapFallback();
                m_brdfLut = CreateBrdfLut();
                m_skybox = m_cubemapFallback;
                m_irradiance = m_cubemapFallback;
                m_prefilter = m_cubemapFallback;
            }

            // Set
            for (size_t i = 0; i < m_sets.size(); ++i)
            {
                m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                WriteSet(static_cast<uint32_t>(i), true);
            }
        }

        RenderFrameData::~RenderFrameData()
        {
            DestroyTextureSafe(m_brdfLut);
            DestroyTextureSafe(m_skybox);
            DestroyTextureSafe(m_irradiance);
            DestroyTextureSafe(m_prefilter);
            Resource::DestroyTexture(m_resources, m_cubemapFallback);

            for (auto& handle : m_uboHandles)
            {
                m_resources.DestroyBuffer(handle);
            }
        }

        void RenderFrameData::UpdateUbo(uint32_t frameIndex, const Scene::Camera& camera, const Scene::Light& light)
        {
            Gpu::PerFrame ubo{};
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 proj = camera.GetProjectMatrix(m_swapChain.Aspect());
            ubo.m_camera.m_viewProj = proj * view;
            ubo.m_camera.m_skyboxViewProj = proj * glm::mat4(glm::mat3(view));
            ubo.m_camera.m_position = glm::vec4(camera.GetPosition(), 1.0f);
            ubo.m_light.m_position = glm::vec4(light.GetPosition(), 0.0f);
            ubo.m_light.m_colorIntensity = glm::vec4(light.GetColor(), light.GetIntensity());

            m_resources.WriteBuffer(m_uboHandles[frameIndex], &ubo, sizeof(ubo));
        }

        void RenderFrameData::UpdateSkybox(const Scene::Skybox& sceneSkybox)
        {
            const auto& skyboxTex = sceneSkybox.GetSkybox();
            if (skyboxTex.GetId() != m_lastSkyboxId)
            {
                m_lastSkyboxId = skyboxTex.GetId();

                DestroyTextureSafe(m_skybox);
                DestroyTextureSafe(m_irradiance);
                DestroyTextureSafe(m_prefilter);

                if (!skyboxTex.IsValid())
                {
                    m_skybox = m_cubemapFallback;
                    m_irradiance = m_cubemapFallback;
                    m_prefilter = m_cubemapFallback;
                }
                else
                {
                    m_skybox = CreateSkybox(*skyboxTex);
                    m_irradiance = CreateIrradianceMap(m_skybox);
                    m_prefilter = CreatePrefilterEnvMap(m_skybox);

                    Core::Log::Info("[Renderer] Create skybox cubemap: ", skyboxTex->m_name, ", ",
                        skyboxTex->m_width, "x", skyboxTex->m_height, " -> ",
                        Resource::kCubemapFaceSize, "x", Resource::kCubemapFaceSize, "x6");
                    Core::Log::Info("[Renderer] Create irradiance map: ",
                        Resource::kIrradianceSize, "x", Resource::kIrradianceSize, "x6");
                    Core::Log::Info("[Renderer] Create prefilter env map: ",
                        Resource::kPrefilterBaseSize, "x", Resource::kPrefilterBaseSize, "x6");
                }

                m_setDirtyCount = Rhi::kMaxFramesInFlight;

                KITA_LOG_DEBUG("[Renderer] Update per frame descriptor set: textures changed");
            }
        }

        void RenderFrameData::RefreshSet(uint32_t frameIndex)
        {
            if (m_setDirtyCount > 0)
            {
                WriteSet(frameIndex);
                --m_setDirtyCount;
            }
        }

        VkDescriptorSetLayout RenderFrameData::GetSetLayout() const
        {
            return m_descriptorMgr.GetLayout(kLayoutType);
        }

        Resource::RenderTexture RenderFrameData::CreateBrdfLut() const
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

        Resource::RenderTexture RenderFrameData::CreateSkybox(const Resource::TextureAsset& texture) const
        {
            const VkFormat equirectFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

            // 1. Create equirect texture (SHADER_READ_ONLY_OPTIMAL)
            Resource::RenderTexture equirect = CreateEquirectTexture(texture, equirectFormat);

            // 2.1 Create cubemap (sample)
            Resource::RenderTexture cubemap = Resource::CreateCubemapTexture(m_resources,
                Resource::kCubemapFaceSize, m_context.HdrFormat(), Rhi::CalculateMipLevels(Resource::kCubemapFaceSize, Resource::kCubemapFaceSize),
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                m_resources.CreateSamplerLinearClampMip());

            // 2.2 Create storage image view for conversion
            VkImageSubresourceRange storageRange{};
            storageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            storageRange.baseMipLevel = 0;
            storageRange.levelCount = 1;
            storageRange.baseArrayLayer = 0;
            storageRange.layerCount = 6;
            Resource::RenderImageViewHandle storageImageViewHandle = m_resources.CreateImageView(cubemap.m_imageHandle, VK_IMAGE_VIEW_TYPE_CUBE, storageRange);

            // 3. GPU conversion: dispatch equirect_to_cubemap compute shader
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 6;

            Resource::ComputeConversion::Output output{};
            output.m_image = cubemap.m_imageHandle;
            output.m_imageView = storageImageViewHandle;
            output.m_range = range;
            output.m_finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            output.m_finalStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            output.m_finalAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT;

            Resource::ComputeConversion::Input input{};
            input.m_imageView = equirect.m_imageViewHandle;
            input.m_sampler = equirect.m_samplerHandle;

            m_skyboxConversion->Dispatch(output, { (Resource::kCubemapFaceSize + 7) / 8, (Resource::kCubemapFaceSize + 7) / 8, 6 },
                { input });

            // 4. Destroy conversion resources
            m_resources.DestroyImageView(storageImageViewHandle);
            Resource::DestroyTexture(m_resources, equirect);

            // 5. Generate cubemap mipmaps
            Resource::RenderImage* cubemapImage = m_resources.GetImage(cubemap.m_imageHandle);
            assert(cubemapImage && "Scene::Skybox: cubemap image is invalid handle");
            range.baseMipLevel = 1;
            range.levelCount = cubemapImage->m_mipLevels - 1;
            m_resources.TransitionImageLayout(cubemap.m_imageHandle,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                range);

            m_resources.GenerateImageMipmaps(cubemap.m_imageHandle);

            return cubemap;
        }

        Resource::RenderTexture RenderFrameData::CreateIrradianceMap(const Resource::RenderTexture& sourceCubemap) const
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
            Gpu::IrradiancePC push{};
            push.m_envMip = static_cast<float>(
                std::log2(static_cast<double>(Resource::kCubemapFaceSize) / Resource::kIrradianceSize));

            m_irradianceConversion->Dispatch(output, { (Resource::kIrradianceSize + 7) / 8, (Resource::kIrradianceSize + 7) / 8, 6 },
                { input }, &push);

            return irradiance;
        }

        Resource::RenderTexture RenderFrameData::CreatePrefilterEnvMap(const Resource::RenderTexture& sourceCubemap) const
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

                Gpu::PrefilterPC push{};
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

        void RenderFrameData::DestroyTextureSafe(Resource::RenderTexture& texture) const
        {
            if (texture != m_cubemapFallback)
            {
                Resource::DestroyTexture(m_resources, texture);
            }
        }

        void RenderFrameData::WriteSet(uint32_t frameIndex, bool writeUbo) const
        {
            Rhi::DescriptorWriter writer(m_resources, m_context.Device());
            if (writeUbo)
            {
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    m_uboHandles[frameIndex], 0, sizeof(Gpu::PerFrame));
            }
            writer.WriteImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_brdfLut.m_imageViewHandle, m_brdfLut.m_samplerHandle)
                .WriteImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_skybox.m_imageViewHandle, m_skybox.m_samplerHandle)
                .WriteImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_irradiance.m_imageViewHandle, m_irradiance.m_samplerHandle)
                .WriteImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_prefilter.m_imageViewHandle, m_prefilter.m_samplerHandle);

            writer.UpdateSet(m_sets[frameIndex]);
        }

        Resource::RenderTexture RenderFrameData::CreateCubemapFallback() const
        {
            return Resource::CreateCubemapFallback(m_resources, m_context.HdrFormat());
        }

        Resource::RenderTexture RenderFrameData::CreateEquirectTexture(const Resource::TextureAsset& texture, VkFormat format) const
        {
            // Hdr asset: m_bytes holds float RGBA pixels
            const float* pixels = reinterpret_cast<const float*>(texture.m_bytes.data());
            const uint32_t width = texture.m_width;
            const uint32_t height = texture.m_height;
            const uint32_t mipLevels = 1;

            return Resource::Create2DTextureWithData(m_resources, pixels, texture.m_bytes.size(),
                width, height, format, mipLevels,
                m_resources.CreateSamplerEquirect());
        }
    }
}
