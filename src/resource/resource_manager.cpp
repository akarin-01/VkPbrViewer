#include "resource_manager.h"

#include "core/log.h"
#include "rhi/context.h"
#include "rhi/one_shot_command.h"
#include "rhi/utils.h"
#include "resource/asset_manager.h"
#include "resource/asset_types.h"
#include "resource/descriptor_manager.h"
#include "resource/environment_baker.h"
#include "resource/graveyard.h"
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
            const AssetManager& assetMgr)
            : m_context(context),
            m_assetMgr(assetMgr),
            m_descriptorMgr(std::make_unique<DescriptorManager>(context)),
            m_graveyard(std::make_unique<Graveyard>(context, *m_descriptorMgr)),
            m_bufferTable([this](BufferRhi&& buffer)
                {
                    if (buffer.m_mapped)
                    {
                        vkUnmapMemory(m_context.Device(), buffer.m_memory);
                    }
                    KITA_LOG_DEBUG("[Resource] Release buffer: ", buffer.m_size, " bytes");
                    m_graveyard->PushBuffer(buffer.m_buffer);
                    m_graveyard->PushMemory(buffer.m_memory);
                }),
            m_descriptorSetTable([this](DescriptorSetRhi&& set)
                {
                    KITA_LOG_DEBUG("[Resource] Release descriptor set");
                    m_graveyard->PushDescriptorSet(set.m_set, set.m_layout);
                }),
            m_imageCache("image resource", [this](ImageRhi&& image)
                {
                    KITA_LOG_DEBUG("[Resource] Release image: ", image.m_extent.width, "x",
                        image.m_extent.height, ", ", image.m_mipLevels, " mips");
                    m_graveyard->PushImage(image.m_image);
                    m_graveyard->PushMemory(image.m_memory);
                }),
            m_imageViewTable([this](ImageViewRhi&& imageView)
                {
                    KITA_LOG_DEBUG("[Resource] Release image view");
                    // m_image releases automatically when the temporary dies
                    // (Handle dtor), while every table is still alive
                    m_graveyard->PushImageView(imageView.m_imageView);
                }),
            m_samplerCache("sampler resource", [this](SamplerRhi&& sampler)
                {
                    KITA_LOG_DEBUG("[Resource] Release sampler");
                    m_graveyard->PushSampler(sampler.m_sampler);
                }),
            m_meshCache("mesh resource", [](MeshResource&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh resource: vb ",
                        mesh.m_vertexBuffer->m_size, " + ib ", mesh.m_indexBuffer->m_size, " bytes");
                }),
            m_environmentBaker(std::make_unique<EnvironmentBaker>(m_context, *m_descriptorMgr))
        {
            m_fallbacks = CreateMaterialFallbacks();
            m_cubemapFallback = CreateCubemapFallback();
            m_brdfLut = CreateTexture(m_environmentBaker->BakeBrdfLut());
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

        DescriptorSetRhi::Handle ResourceManager::CreateDescriptorSet(DescriptorSetRhi::Type type)
        {
            DescriptorSetRhi set{};
            set.m_set = m_descriptorMgr->Allocate(type);
            set.m_layout = type;

            KITA_LOG_DEBUG("[Resource] Create descriptor set");
            return m_descriptorSetTable.Create(std::move(set));
        }

        UboResource ResourceManager::CreateUbo(const BufferDesc& desc)
        {
            return UboResource{ CreateBuffer(desc) };
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

        std::array<TextureResource, 3> ResourceManager::CreateEnvironments(ResourceId equirectId)
        {
            TextureResource skybox{};
            TextureResource irradiance{};
            TextureResource prefilter{};

            const TextureAsset* asset = m_assetMgr.GetTexture(equirectId);
            if (asset)
            {
                skybox = CreateTexture(m_environmentBaker->BakeSkybox(CreateEquirect(*asset)));
                irradiance = CreateTexture(m_environmentBaker->BakeIrradiance(skybox));
                prefilter = CreateTexture(m_environmentBaker->BakePrefilter(skybox));
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

                skybox = CreateTexture(m_cubemapFallback, imageViewDesc, samplerDesc);
                irradiance = CreateTexture(m_cubemapFallback, imageViewDesc, samplerDesc);
                prefilter = CreateTexture(m_cubemapFallback, imageViewDesc, samplerDesc);
            }

            return { std::move(skybox), std::move(irradiance), std::move(prefilter) };
        }

        TextureResource ResourceManager::CreateFallback(MaterialSlot slot, const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc)
        {
            return CreateTexture(m_fallbacks[static_cast<size_t>(slot)], imageViewDesc, samplerDesc);
        }

        void ResourceManager::FlushGraveyard()
        {
            m_graveyard->Flush();
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

        TextureResource ResourceManager::CreateTexture(BakedTexture baked)
        {
            KITA_LOG_DEBUG("[Resource] Create image: ", baked.m_image.m_extent.width, "x",
                baked.m_image.m_extent.height, ", ", baked.m_image.m_mipLevels, " mips");
            const ImageRhi::Handle image = m_imageCache.Create(std::move(baked.m_image));
            return CreateTexture(image, baked.m_viewDesc, baked.m_samplerDesc);
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
    }
}
