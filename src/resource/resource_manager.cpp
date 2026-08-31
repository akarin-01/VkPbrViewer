#include "resource_manager.h"

#include "core/log.h"
#include "rhi/context.h"
#include "rhi/one_shot_command.h"
#include "rhi/utils.h"
#include "resource/asset_manager.h"
#include "resource/asset_types.h"
#include "resource/descriptor_manager.h"
#include "resource/descriptor_writer.h"
#include "resource/environment_baker.h"
#include "resource/gpu_layouts.h"
#include "resource/resource_utils.h"

#include <cmath>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace
        {
            const char* ToString(SamplerDesc::MipMode mode)
            {
                switch (mode)
                {
                case SamplerDesc::MipMode::None:    return "None";
                case SamplerDesc::MipMode::Nearest: return "Nearest";
                case SamplerDesc::MipMode::Linear:  return "Linear";
                default:                            return "Unknown";
                }
            }

            const char* ToString(VkFilter filter)
            {
                switch (filter)
                {
                case VK_FILTER_NEAREST: return "Nearest";
                case VK_FILTER_LINEAR:  return "Linear";
                default:                return "Unknown";
                }
            }

            const char* ToString(VkSamplerAddressMode mode)
            {
                switch (mode)
                {
                case VK_SAMPLER_ADDRESS_MODE_REPEAT:          return "Repeat";
                case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: return "MirroredRepeat";
                case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:   return "ClampToEdge";
                case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return "ClampToBorder";
                default:                                      return "Unknown";
                }
            }

            VkFormat ToImageFormat(TextureAsset::Type type)
            {
                switch (type)
                {
                case TextureAsset::Type::Srgb:              return VK_FORMAT_R8G8B8A8_SRGB;
                case TextureAsset::Type::Normal:            return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureAsset::Type::MetallicRoughness: return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureAsset::Type::Linear:            return VK_FORMAT_R8_UNORM;
                case TextureAsset::Type::Hdr:               return VK_FORMAT_R32G32B32A32_SFLOAT;
                default:                                    return VK_FORMAT_UNDEFINED;
                }
            }
        }

        ResourceManager::ResourceManager(const Rhi::Context& context,
            const AssetManager& assetMgr,
            DescriptorManager& descriptorMgr)
            : m_context(context),
            m_assetMgr(assetMgr),
            m_descriptorMgr(descriptorMgr),
            m_graveyard(context),
            m_bufferTable([this](BufferRhi&& buffer)
                {
                    if (buffer.m_mapped)
                    {
                        vkUnmapMemory(m_context.Device(), buffer.m_memory);
                    }
                    KITA_LOG_DEBUG("[Resource] Release buffer: ", buffer.m_size, " bytes");
                    m_graveyard.PushBuffer(buffer.m_buffer);
                    m_graveyard.PushMemory(buffer.m_memory);
                }),
            m_imageCache("image resource", [this](ImageRhi&& image)
                {
                    KITA_LOG_DEBUG("[Resource] Release image: ", image.m_extent.width, "x",
                        image.m_extent.height, ", ", image.m_mipLevels, " mips");
                    m_graveyard.PushImage(image.m_image);
                    m_graveyard.PushMemory(image.m_memory);
                }),
            m_imageViewTable([this](ImageViewRhi&& imageView)
                {
                    KITA_LOG_DEBUG("[Resource] Release image view");
                    // m_image releases automatically when the temporary dies
                    // (Handle dtor), while every table is still alive.
                    m_graveyard.PushImageView(imageView.m_imageView);
                }),
            m_samplerCache("sampler resource", [this](SamplerRhi&& sampler)
                {
                    KITA_LOG_DEBUG("[Resource] Release sampler");
                    m_graveyard.PushSampler(sampler.m_sampler);
                }),
            m_meshCache("mesh resource", [](MeshResource&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh resource: vb ",
                        mesh.m_vertexBuffer->m_size, " + ib ", mesh.m_indexBuffer->m_size, " bytes");
                }),
            m_materialCache("per material set", [](PerMaterialSet&&)
                {
                    KITA_LOG_DEBUG("[Resource] Release per material set");
                    // Texture handles release with the entry; each pushes into
                    // its own graveyard queue through the tables above.
                })
        {
            m_fallbacks = CreateMaterialFallbacks();
            m_cubemapFallback = CreateCubemapFallback();

            m_environmentBaker =
                std::make_unique<EnvironmentBaker>(m_context, m_descriptorMgr, *this);
            m_brdfLut = m_environmentBaker->BakeBrdfLut();
        }

        ResourceManager::~ResourceManager() = default;

        BufferRhi::Handle ResourceManager::CreateBuffer(const BufferDesc& desc, const void* data, size_t size)
        {
            BufferRhi buffer = ResourceUtils::CreateBufferRhi(m_context, desc, data, size);

            KITA_LOG_DEBUG("[Resource] Create buffer: ", desc.m_size, " bytes");
            return m_bufferTable.Create(std::move(buffer));
        }

        ImageRhi::Handle ResourceManager::CreateImage(const ImageDesc& desc, const void* data, size_t size)
        {
            ImageRhi image = ResourceUtils::CreateImageRhi(m_context, desc, data, size);

            KITA_LOG_DEBUG("[Resource] Create image: ", desc.m_extent.width, "x",
                desc.m_extent.height, ", ", desc.m_mipLevels, " mips");
            return m_imageCache.Create(std::move(image));
        }

        ImageRhi::Handle ResourceManager::GetOrCreateImage(ResourceId textureId)
        {
            return m_imageCache.GetOrCreate(textureId, [this](ResourceId textureKey) -> std::optional<ImageRhi>
                {
                    auto asset = m_assetMgr.GetTexture(textureKey);
                    if (!asset)
                    {
                        // Invalid texture id, return invalid handle
                        return std::nullopt;
                    }
                    return CreateImage(*asset);
                });
        }

        ImageViewRhi::Handle ResourceManager::CreateImageView(const ImageViewDesc& desc, ImageRhi::Handle image)
        {
            if (!image)
            {
                throw std::runtime_error("CreateImageView: invalid image handle");
            }

            ImageViewRhi imageView = ResourceUtils::CreateImageViewRhi(m_context, *image, desc);
            imageView.m_image = std::move(image);

            KITA_LOG_DEBUG("[Resource] Create image view");
            return m_imageViewTable.Create(std::move(imageView));
        }

        SamplerRhi::Handle ResourceManager::GetOrCreateSampler(const SamplerDesc& desc)
        {
            return m_samplerCache.GetOrCreate(desc, [this](const SamplerDesc& d) -> std::optional<SamplerRhi>
                {
                    SamplerRhi sampler = ResourceUtils::CreateSamplerRhi(m_context, d);

                    KITA_LOG_DEBUG("[Resource] Create sampler resource: filter ", ToString(d.m_magFilter),
                        "/", ToString(d.m_minFilter), ", mip ", ToString(d.m_mipMode),
                        ", address ", ToString(d.m_addressModeU), ", anisotropy ", d.m_anisotropy);
                    return sampler;
                });
        }

        TextureResource ResourceManager::CreateTexture(ResourceId textureId, const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc)
        {
            const ImageRhi::Handle image = GetOrCreateImage(textureId);
            if (!image)
            {
                // Invalid texture id: empty texture, matching GetOrCreate* convention
                return TextureResource{};
            }

            const TextureResource texture = CreateTexture(image, imageViewDesc, samplerDesc);

            Core::Log::Info("[Resource] Create texture resource: texture asset(", textureId, ")");
            return texture;
        }

        TextureResource ResourceManager::CreateTexture(const ImageDesc& imageDesc, const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc, const void* data, size_t size)
        {
            const TextureResource texture = CreateTexture(CreateImage(imageDesc, data, size), imageViewDesc, samplerDesc);

            Core::Log::Info("[Resource] Create texture resource: image data ", size, " bytes, ",
                imageDesc.m_extent.width, "x", imageDesc.m_extent.height);
            return texture;
        }

        TextureResource ResourceManager::CreateTexture(ImageRhi::Handle image, const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc)
        {
            TextureResource texture{};
            texture.m_image = std::move(image);
            texture.m_imageView = CreateImageView(imageViewDesc, texture.m_image);
            texture.m_sampler = GetOrCreateSampler(samplerDesc);
            return texture;
        }

        MeshResource::Handle ResourceManager::GetOrCreateMesh(ResourceId meshId)
        {
            return m_meshCache.GetOrCreate(meshId, [this](ResourceId meshKey) -> std::optional<MeshResource>
                {
                    auto asset = m_assetMgr.GetMesh(meshKey);
                    if (!asset)
                    {
                        // Invalid mesh id, return invalid handle
                        return std::nullopt;
                    }
                    return CreateMesh(*asset);
                });
        }

        TargetResource ResourceManager::CreateTarget(const TargetDesc& desc)
        {
            // Color (MSAA) + resolve + depth. Color and resolve share the
            // format: vkCmdResolveImage requires identical src/dst formats.
            ImageDesc colorDesc{};
            colorDesc.m_extent = { desc.m_extent.width, desc.m_extent.height, 1 };
            colorDesc.m_format = desc.m_colorFormat;
            colorDesc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorDesc.m_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            colorDesc.m_samples = desc.m_msaaSamples;
            colorDesc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            ImageDesc resolveDesc = colorDesc;
            resolveDesc.m_samples = VK_SAMPLE_COUNT_1_BIT;
            resolveDesc.m_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

            ImageDesc depthDesc{};
            depthDesc.m_extent = { desc.m_extent.width, desc.m_extent.height, 1 };
            depthDesc.m_format = desc.m_depthFormat;
            depthDesc.m_aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthDesc.m_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depthDesc.m_samples = desc.m_msaaSamples;   // MSAA depth matches color
            depthDesc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            ImageViewDesc viewDesc{};
            viewDesc.m_type = VK_IMAGE_VIEW_TYPE_2D;
            viewDesc.m_fullRange = true;

            TargetResource target{};
            target.m_colorTexture = CreateTexture(colorDesc, viewDesc, desc.m_samplerDesc);
            target.m_resolveTexture = CreateTexture(resolveDesc, viewDesc, desc.m_samplerDesc);
            target.m_depthTexture = CreateTexture(depthDesc, viewDesc, desc.m_samplerDesc);

            Core::Log::Info("[Resource] Create target: ", desc.m_extent.width, "x", desc.m_extent.height,
                ", samples ", desc.m_msaaSamples);
            return target;
        }

        PerObjectSet ResourceManager::CreatePerObjectSet()
        {
            constexpr DescriptorLayoutType kLayoutType = DescriptorLayoutType::PerObject;

            PerObjectSet perObject{};
            perObject.m_layout = m_descriptorMgr.GetLayout(kLayoutType);

            Resource::BufferDesc desc{};
            desc.m_size = sizeof(Gpu::PerObject);
            desc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.m_properties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            desc.m_mapped = true;

            for (size_t i = 0; i < perObject.m_ubos.size(); ++i)
            {
                perObject.m_ubos[i] = CreateUbo(desc);
            }

            for (size_t i = 0; i < perObject.m_sets.size(); ++i)
            {
                perObject.m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    perObject.m_ubos[i].GetBuffer(), 0, sizeof(Gpu::PerObject))
                    .UpdateSet(perObject.m_sets[i]);
            }

            KITA_LOG_DEBUG("[Resource] Create per-object set: ", perObject.m_ubos.size(), " ubo slots, ",
                sizeof(Gpu::PerObject), " bytes, ", perObject.m_sets.size(), " sets");
            return perObject;
        }

        PerMaterialSet::Handle ResourceManager::GetOrCreatePerMaterialSet(const MaterialDesc& desc)
        {
            return m_materialCache.GetOrCreate(desc, [this](const MaterialDesc& d) -> std::optional<PerMaterialSet>
                {
                    return CreatePerMaterialSet(d);
                });
        }

        PostProcessSet ResourceManager::CreatePostProcessSet(const TextureResource& texture)
        {
            constexpr DescriptorLayoutType kLayoutType = DescriptorLayoutType::PostProcess;

            PostProcessSet postProcess{};
            postProcess.m_layout = m_descriptorMgr.GetLayout(kLayoutType);

            Resource::BufferDesc desc{};
            desc.m_size = sizeof(Gpu::PostProcess);
            desc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            desc.m_mapped = true;

            for (size_t i = 0; i < postProcess.m_ubos.size(); ++i)
            {
                postProcess.m_ubos[i] = CreateUbo(desc);
            }

            for (size_t i = 0; i < postProcess.m_sets.size(); ++i)
            {
                postProcess.m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    postProcess.m_ubos[i].GetBuffer(), 0, sizeof(Gpu::PostProcess))
                    .WriteImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.GetImageView(), texture.GetSampler())
                    .UpdateSet(postProcess.m_sets[i]);
            }

            KITA_LOG_DEBUG("[Resource] Create post process set: ", postProcess.m_ubos.size(), " ubo slots, ",
                sizeof(Gpu::PostProcess), " bytes, ", postProcess.m_sets.size(), " sets, 1 texture slot");
            return postProcess;
        }

        PerFrameSet ResourceManager::CreatePerFrameSet(ResourceId equirectId)
        {
            constexpr DescriptorLayoutType kLayoutType = DescriptorLayoutType::PerFrame;

            PerFrameSet perFrame{};
            perFrame.m_layout = m_descriptorMgr.GetLayout(kLayoutType);

            Resource::BufferDesc desc{};
            desc.m_size = sizeof(Gpu::PerFrame);
            desc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            desc.m_mapped = true;

            for (size_t i = 0; i < perFrame.m_ubos.size(); ++i)
            {
                perFrame.m_ubos[i] = CreateUbo(desc);
            }

            const TextureAsset* asset = m_assetMgr.GetTexture(equirectId);
            if (asset)
            {
                perFrame.m_skybox = m_environmentBaker->BakeSkybox(CreateEquirect(*asset));
                perFrame.m_irradiance = m_environmentBaker->BakeIrradiance(perFrame.m_skybox);
                perFrame.m_prefilter = m_environmentBaker->BakePrefilter(perFrame.m_skybox);
            }
            else
            {
                ImageViewDesc imageViewDesc{};
                imageViewDesc.m_type = VK_IMAGE_VIEW_TYPE_CUBE;
                imageViewDesc.m_fullRange = true;

                SamplerDesc samplerDesc{};
                samplerDesc.m_magFilter = VK_FILTER_LINEAR;
                samplerDesc.m_minFilter = VK_FILTER_LINEAR;
                samplerDesc.m_mipMode = SamplerDesc::MipMode::None;
                samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

                perFrame.m_skybox = CreateTexture(m_cubemapFallback, imageViewDesc, samplerDesc);
                perFrame.m_irradiance = CreateTexture(m_cubemapFallback, imageViewDesc, samplerDesc);
                perFrame.m_prefilter = CreateTexture(m_cubemapFallback, imageViewDesc, samplerDesc);
            }

            for (size_t i = 0; i < perFrame.m_sets.size(); ++i)
            {
                perFrame.m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    perFrame.m_ubos[i].GetBuffer(), 0, sizeof(Gpu::PerFrame))
                    .WriteImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_brdfLut.GetImageView(), m_brdfLut.GetSampler())
                    .WriteImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, perFrame.m_skybox.GetImageView(), perFrame.m_skybox.GetSampler())
                    .WriteImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, perFrame.m_irradiance.GetImageView(), perFrame.m_irradiance.GetSampler())
                    .WriteImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, perFrame.m_prefilter.GetImageView(), perFrame.m_prefilter.GetSampler())
                    .UpdateSet(perFrame.m_sets[i]);
            }

            KITA_LOG_DEBUG("[Resource] Create per frame set: ", perFrame.m_ubos.size(), " ubo slots, ",
                sizeof(Gpu::PerFrame), " bytes, ", perFrame.m_sets.size(), " sets, 4 texture slots");
            return perFrame;
        }

        void ResourceManager::FlushGraveyard()
        {
            m_graveyard.Flush();
        }

        ImageRhi ResourceManager::CreateImage(const TextureAsset& asset)
        {
            ImageDesc desc{};
            desc.m_extent = { asset.m_width, asset.m_height, 1 };
            desc.m_format = ToImageFormat(asset.m_type);
            desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            desc.m_mipLevels = ResourceUtils::CalculateMipLevels(asset.m_width, asset.m_height);
            desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;

            KITA_LOG_DEBUG("[Resource] Create image: ", desc.m_extent.width, "x",
                desc.m_extent.height, ", ", desc.m_mipLevels, " mips");
            return ResourceUtils::CreateImageRhi(m_context, desc, asset.m_bytes.data(), asset.m_bytes.size());
        }

        UboResource ResourceManager::CreateUbo(const BufferDesc& desc)
        {
            return UboResource{ CreateBuffer(desc) };
        }

        MeshResource ResourceManager::CreateMesh(const MeshAsset& asset)
        {
            MeshResource mesh{};

            // Vertex
            {
                BufferDesc desc{};
                desc.m_size = asset.GetVertexDataSize();
                desc.m_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                desc.m_mapped = false;

                mesh.m_vertexBuffer = CreateBuffer(desc, asset.GetVertexData(), asset.GetVertexDataSize());
            }

            // Index
            {
                BufferDesc desc{};
                desc.m_size = asset.GetIndexDataSize();
                desc.m_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                desc.m_mapped = false;

                mesh.m_indexBuffer = CreateBuffer(desc, asset.GetIndexData(), asset.GetIndexDataSize());
            }

            mesh.m_indexCount = static_cast<uint32_t>(asset.GetIndexCount());

            Core::Log::Info("[Resource] Create mesh resource: ", asset.m_name, ", vb ",
                asset.GetVertexDataSize(), " bytes, ib ", asset.GetIndexDataSize(), " bytes");

            return mesh;
        }

        TextureResource ResourceManager::CreateEquirect(const TextureAsset& asset)
        {
            ImageDesc desc{};
            desc.m_extent = { asset.m_width, asset.m_height, 1 };
            desc.m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
            desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            SamplerDesc samplerDesc{};
            samplerDesc.m_magFilter = VK_FILTER_LINEAR;
            samplerDesc.m_minFilter = VK_FILTER_LINEAR;
            samplerDesc.m_mipMode = SamplerDesc::MipMode::None;
            samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            return CreateTexture(desc, {}, samplerDesc, asset.m_bytes.data(), asset.m_bytes.size());
        }

        std::array<ImageRhi::Handle, kMaterialSlotCount> ResourceManager::CreateMaterialFallbacks()
        {
            constexpr uint8_t white[] = { 255, 255, 255, 255 };
            constexpr uint8_t black[] = { 0, 0, 0, 255 };
            constexpr uint8_t flat[] = { 128, 128, 255, 255 };

            const auto makeFallback = [this](VkFormat format, const uint8_t* bytes, size_t size)
                {
                    ImageDesc desc{};
                    desc.m_extent = { 1, 1, 1 };
                    desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
                    desc.m_format = format;
                    return CreateImage(desc, bytes, size);
                };

            std::array<ImageRhi::Handle, kMaterialSlotCount> fallbacks{};
            fallbacks[static_cast<size_t>(Resource::MaterialSlot::Albedo)] =
                makeFallback(VK_FORMAT_R8G8B8A8_SRGB, white, 4);    // opaque white
            fallbacks[static_cast<size_t>(Resource::MaterialSlot::Normal)] =
                makeFallback(VK_FORMAT_R8G8B8A8_UNORM, flat, 4);    // flat tangent-space normal
            fallbacks[static_cast<size_t>(Resource::MaterialSlot::MetallicRoughness)] =
                makeFallback(VK_FORMAT_R8G8B8A8_UNORM, white, 4);   // metal 1, rough 0
            fallbacks[static_cast<size_t>(Resource::MaterialSlot::AO)] =
                makeFallback(VK_FORMAT_R8_UNORM, white, 1);         // no occlusion
            fallbacks[static_cast<size_t>(Resource::MaterialSlot::Emissive)] =
                makeFallback(VK_FORMAT_R8G8B8A8_SRGB, black, 4);    // no emission
            return fallbacks;
        }

        ImageRhi::Handle ResourceManager::CreateCubemapFallback()
        {
            ImageDesc desc{};
            desc.m_extent = { 1, 1, 1 };
            desc.m_arrayLayers = 6;
            desc.m_format = m_context.HdrFormat();
            desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            desc.m_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            std::array<uint32_t, 24> blackData{};   // 6 layers * 1 texel * RGBA32F zeros
            return CreateImage(desc, blackData.data(), blackData.size() * sizeof(uint32_t));
        }

        PerMaterialSet ResourceManager::CreatePerMaterialSet(const MaterialDesc& desc)
        {
            // Material sampling policy, spelled out at the only assembly point.
            ImageViewDesc imageViewDesc{};
            imageViewDesc.m_type = VK_IMAGE_VIEW_TYPE_2D;
            imageViewDesc.m_fullRange = true;

            SamplerDesc samplerDesc{};
            samplerDesc.m_magFilter = VK_FILTER_LINEAR;
            samplerDesc.m_minFilter = VK_FILTER_LINEAR;
            samplerDesc.m_mipMode = SamplerDesc::MipMode::Linear;
            samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            PerMaterialSet material{};
            material.m_layout = m_descriptorMgr.GetLayout(DescriptorLayoutType::PerMaterial);
            material.m_set = m_descriptorMgr.Allocate(DescriptorLayoutType::PerMaterial);

            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                const TextureResource tex = CreateTexture(desc.m_textureIds[i], imageViewDesc, samplerDesc);
                material.m_textures[i] = tex.IsEmpty() ? CreateTexture(m_fallbacks[i], imageViewDesc, samplerDesc) : tex;
            }

            DescriptorWriter writer(m_context.Device());
            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                writer.WriteImage(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    material.m_textures[i].GetImageView(), material.m_textures[i].GetSampler());
            }
            writer.UpdateSet(material.m_set);

            KITA_LOG_DEBUG("[Resource] Create per-material set: ", Resource::kMaterialSlotCount,
                " texture slots, 1 set");
            return material;
        }
    }
}
