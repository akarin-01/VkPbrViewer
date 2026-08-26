#include "render_skybox_data.h"
#include "core/log.h"
#include "rhi/context.h"
#include "resource/compute_conversion.h"
#include "resource/resources.h"
#include "resource/texture_set.h"
#include "scene/skybox.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cassert>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr uint32_t kCubemapFaceSize = 2048;
        }

        RenderSkyboxData::RenderSkyboxData(const Rhi::RenderContext& context,
            Resource::RenderResources& resources,
            const Resource::DescriptorAllocator& descriptorAllocator)
            : m_context(context),
            m_resources(resources),
            m_descriptorAllocator(descriptorAllocator)
        {
            m_textureSet = std::make_unique<TextureSet>(m_context, m_resources, m_descriptorAllocator,
                TextureArray{ Resource::CreateCubemapFallback(m_resources, m_context.HdrFormat()) });
            m_conversion = std::make_unique<Resource::ComputeConversion>(m_context, m_resources, m_descriptorAllocator,
                1, "assets/shaders/equirect_to_cubemap_comp.spv", 0);
        }

        RenderSkyboxData::~RenderSkyboxData()
        {
            m_conversion.reset();
            m_textureSet.reset();
        }

        void RenderSkyboxData::Update(uint32_t frameIndex, const Scene::Skybox& sceneSkybox)
        {
            if (sceneSkybox.GetRevision() != m_lastSyncedRevision)
            {
                m_lastSyncedRevision = sceneSkybox.GetRevision();

                TextureArray updatedTexs{};
                if (sceneSkybox.IsEmpty())
                {
                    updatedTexs = m_textureSet->GetFallbacks();
                }
                else
                {
                    updatedTexs[0] = CreateCubemap(sceneSkybox);

                    Core::Log::Info("[Renderer] Create skybox cubemap: ", sceneSkybox.GetName(), ", ",
                        sceneSkybox.GetWidth(), "x", sceneSkybox.GetHeight(), " -> ",
                        kCubemapFaceSize, "x", kCubemapFaceSize, "x6");
                }

                m_textureSet->Update(updatedTexs);
                KITA_LOG_DEBUG("[Renderer] Update skybox descriptor set: textures changed");
            }

            m_textureSet->RefreshSet(frameIndex);
        }

        VkDescriptorSetLayout RenderSkyboxData::GetSetLayout() const
        {
            return m_textureSet->GetLayout();
        }

        const VkDescriptorSet& RenderSkyboxData::GetSet(uint32_t frameIndex) const
        {
            return m_textureSet->GetSet(frameIndex);
        }

        Resource::RenderTexture RenderSkyboxData::GetCubemap() const
        {
            return m_textureSet->GetTextures()[0];
        }

        Resource::RenderTexture RenderSkyboxData::CreateCubemap(const Scene::Skybox& sceneSkybox) const
        {
            const VkFormat equirectFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

            // 1. Create equirect texture (SHADER_READ_ONLY_OPTIMAL)
            Resource::RenderTexture equirect = CreateEquirectTexture(sceneSkybox, equirectFormat);

            // 2.1 Create cubemap (sample)
            Resource::RenderTexture cubemap = Resource::CreateCubemapTexture(m_resources,
                kCubemapFaceSize, m_context.HdrFormat(), Rhi::CalculateMipLevels(kCubemapFaceSize, kCubemapFaceSize),
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

            m_conversion->Dispatch(output, { (kCubemapFaceSize + 7) / 8, (kCubemapFaceSize + 7) / 8, 6 },
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

        Resource::RenderTexture RenderSkyboxData::CreateEquirectTexture(const Scene::Skybox& sceneSkybox, VkFormat format) const
        {
            const auto& pixels = sceneSkybox.GetPixels();
            const uint32_t width = sceneSkybox.GetWidth();
            const uint32_t height = sceneSkybox.GetHeight();
            const uint32_t mipLevels = 1;

            const uint32_t channels = static_cast<uint32_t>(pixels.size() / (width * height));
            assert(channels == 4 && "Scene::Skybox pixels must be RGBA (LoadSkybox forces 4 channels)");

            return Resource::Create2DTextureWithData(m_resources, pixels.data(), pixels.size() * sizeof(float),
                width, height, format, mipLevels,
                m_resources.CreateSamplerEquirect());
        }
    }
}
