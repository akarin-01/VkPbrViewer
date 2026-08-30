#include "resource_manager.h"

#include "core/log.h"
#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "resource/asset_manager.h"
#include "resource/asset_types.h"
#include "resource/resource_utils.h"
#include "resource/gpu_layouts.h"
#include "resource/descriptor_manager.h"

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
            m_bufferTable([this](BufferResource&& buffer)
                {
                    if (buffer.m_mapped)
                    {
                        vkUnmapMemory(m_context.Device(), buffer.m_memory);
                    }
                    KITA_LOG_DEBUG("[Resource] Release buffer: ", buffer.m_size, " bytes");
                    m_graveyard.PushBuffer(buffer.m_buffer);
                    m_graveyard.PushMemory(buffer.m_memory);
                }),
            m_imageTable([this](ImageResource&& image)
                {
                    KITA_LOG_DEBUG("[Resource] Release image: ", image.m_extent.width, "x",
                        image.m_extent.height, ", ", image.m_mipLevels, " mips");
                    m_graveyard.PushImage(image.m_image);
                    m_graveyard.PushMemory(image.m_memory);
                }),
            m_imageViewTable([this](ImageViewResource&& imageView)
                {
                    KITA_LOG_DEBUG("[Resource] Release image view");
                    // m_image releases automatically when the temporary dies
                    // (Handle dtor), while every table is still alive.
                    m_graveyard.PushImageView(imageView.m_imageView);
                }),
            m_samplerTable([this](SamplerResource&& sampler)
                {
                    KITA_LOG_DEBUG("[Resource] Release sampler");
                    m_graveyard.PushSampler(sampler.m_sampler);
                }),
            m_meshTable([](MeshResource&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh resource: vb ",
                        mesh.m_vertexBuffer->m_size, " + ib ", mesh.m_indexBuffer->m_size, " bytes");
                })
        {
        }

        ResourceManager::~ResourceManager() = default;

        BufferResource::Handle ResourceManager::CreateBuffer(const BufferDesc& desc, const void* data, size_t size)
        {
            BufferResource buffer = ResourceUtils::CreateBufferResource(m_context, desc, data, size);

            KITA_LOG_DEBUG("[Resource] Create buffer: ", desc.m_size, " bytes");
            return m_bufferTable.Create(std::move(buffer));
        }

        ImageResource::Handle ResourceManager::CreateImage(const ImageDesc& desc, const void* data, size_t size)
        {
            ImageResource image = ResourceUtils::CreateImageResource(m_context, desc, data, size);

            KITA_LOG_DEBUG("[Resource] Create image: ", desc.m_extent.width, "x",
                desc.m_extent.height, ", ", desc.m_mipLevels, " mips");
            return m_imageTable.Create(std::move(image));
        }

        ImageViewResource::Handle ResourceManager::CreateImageView(const ImageViewDesc& desc, const ImageResource::Handle& image)
        {
            if (!image)
            {
                throw std::runtime_error("CreateImageView: invalid image handle");
            }

            ImageViewResource imageView = ResourceUtils::CreateImageViewResource(m_context, *image, desc);
            imageView.m_image = image;

            KITA_LOG_DEBUG("[Resource] Create image view");
            return m_imageViewTable.Create(std::move(imageView));
        }

        SamplerResource::Handle ResourceManager::GetOrCreateSampler(const SamplerDesc& desc)
        {
            auto it = m_samplerIds.find(desc);
            if (it != m_samplerIds.end())
            {
                const ResourceId id = it->second;
                // The mapping can outlive its entry (every handle released):
                // a stale id falls through and is rebuilt below.
                if (m_samplerTable.Has(id))
                {
                    // Cache hit: add ref
                    KITA_LOG_DEBUG("[Resource] Reuse sampler resource");
                    return m_samplerTable.GetShared(id);
                }
            }

            // Cache miss: create
            SamplerResource sampler = ResourceUtils::CreateSamplerResource(m_context, desc);

            KITA_LOG_DEBUG("[Resource] Create sampler resource: filter ", ToString(desc.m_magFilter),
                "/", ToString(desc.m_minFilter), ", mip ", ToString(desc.m_mipMode),
                ", address ", ToString(desc.m_addressModeU), ", anisotropy ", desc.m_anisotropy);

            SamplerResource::Handle handle = m_samplerTable.Create(std::move(sampler));
            m_samplerIds[desc] = handle.GetId();
            return handle;
        }

        MeshResource::Handle ResourceManager::GetOrCreateMesh(ResourceId meshId)
        {
            auto it = m_meshIds.find(meshId);
            if (it != m_meshIds.end())
            {
                const ResourceId id = it->second;
                // The mapping can outlive its entry (every handle released):
                // a stale id falls through and is rebuilt below.
                if (m_meshTable.Has(id))
                {
                    // Cache hit: add ref
                    KITA_LOG_DEBUG("[Resource] Reuse mesh resource: mesh asset(", meshId, ")");
                    return m_meshTable.GetShared(id);
                }
            }

            // Cache miss: create
            auto asset = m_assetMgr.GetMesh(meshId);
            if (!asset)
            {
                // Invalid mesh id, return invalid handle
                return MeshResource::Handle();
            }

            MeshResource::Handle handle = CreateMesh(*asset);

            m_meshIds[meshId] = handle.GetId();
            return handle;
        }

        ImageResource::Handle ResourceManager::GetOrCreateImage(ResourceId textureId)
        {
            auto it = m_imageIds.find(textureId);
            if (it != m_imageIds.end())
            {
                const ResourceId id = it->second;
                // The mapping can outlive its entry (every handle released):
                // a stale id falls through and is rebuilt below.
                if (m_imageTable.Has(id))
                {
                    // Cache hit: add ref
                    KITA_LOG_DEBUG("[Resource] Reuse image resource: texture asset(", textureId, ")");
                    return m_imageTable.GetShared(id);
                }
            }

            // Cache miss: create
            auto asset = m_assetMgr.GetTexture(textureId);
            if (!asset)
            {
                // Invalid texture id, return invalid handle
                return ImageResource::Handle();
            }

            ImageResource::Handle handle = CreateImage(*asset);
            m_imageIds[textureId] = handle.GetId();
            return handle;
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
                perObject.m_ubos[i] = CreateBuffer(desc);
                perObject.m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                Rhi::V2::DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    perObject.m_ubos[i]->m_buffer, 0, sizeof(Gpu::PerObject))
                    .UpdateSet(perObject.m_sets[i]);
            }

            KITA_LOG_DEBUG("[Resource] Create per-object set: ", perObject.m_ubos.size(), " ubo slots, ",
                sizeof(Gpu::PerObject), " bytes, ", perObject.m_sets.size(), " sets");
            return perObject;
        }

        void ResourceManager::FlushGraveyard()
        {
            m_graveyard.Flush();
        }

        MeshResource::Handle ResourceManager::CreateMesh(const MeshAsset& asset)
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

            return m_meshTable.Create(std::move(mesh));
        }

        ImageResource::Handle ResourceManager::CreateImage(const TextureAsset& asset)
        {
            ImageDesc desc{};
            desc.m_extent = { asset.m_width, asset.m_height, 1 };
            desc.m_format = ToImageFormat(asset.m_type);
            desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            desc.m_mipLevels = ResourceUtils::CalculateMipLevels(asset.m_width, asset.m_height);
            desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;

            return CreateImage(desc, asset.m_bytes.data(), asset.m_bytes.size());
        }
    }
}
