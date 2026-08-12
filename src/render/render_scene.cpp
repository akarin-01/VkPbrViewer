#include "render_scene.h"

#include "scene/scene.h"
#include "scene/vertex.h"
#include "scene/texture.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"

#include <iostream>
#include <glm/glm.hpp>
#include <cassert>

namespace Kita::Pbrv
{
    static VkFormat TypeToFormat(Texture::Type type)
    {
        switch (type)
        {
        case Texture::Type::Albedo:
            return VK_FORMAT_R8G8B8A8_SRGB;
            break;
        case Texture::Type::Normal:
            return VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case Texture::Type::Linear:
            return VK_FORMAT_R8_UNORM;
            break;
        case Texture::Type::Hdr:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
            break;
        default:
            return VK_FORMAT_UNDEFINED;
            break;
        }
    }

    RenderScene::RenderScene(RenderResources& resources, const SwapChain& swapChain)
        : m_resources(resources), m_swapChain(swapChain)
    {
        m_list.m_frame = CreateRenderPerFrame();
        m_list.m_material = CreateRenderMaterial();
    }

    RenderScene::~RenderScene()
    {
        DestroyRenderMesh(m_list.m_mesh);
        DestroyRenderMaterial(m_list.m_material);
        DestroyRenderPerFrame(m_list.m_frame);
    }

    void RenderScene::Update(const Scene& scene, const FrameInfo& frameInfo)
    {
        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& frameIndex = frameInfo.m_frameIndex;
        auto& imageIndex = frameInfo.m_imageIndex;

        // Per frame
        {
            auto& sceneCamera = scene.GetCamera();
            auto& sceneLight = scene.GetLight();

            FrameUbo ubo{};
            ubo.m_viewProj = sceneCamera.GetProjectMatrix(m_swapChain.Aspect())
                * sceneCamera.GetViewMatrix();
            ubo.m_viewPos = glm::vec4(sceneCamera.GetPosition(), 1.0f);
            ubo.m_lightDir = glm::vec4(sceneLight.GetDirection(), 0.0f);
            ubo.m_lightColor = glm::vec4(sceneLight.GetColor(), sceneLight.GetIntensity());

            m_resources.WriteBuffer(m_list.m_frame.m_uboHandles[frameIndex], &ubo, sizeof(ubo));
        }

        // Material
        {
            auto& sceneMat = scene.GetMaterial();
            MaterialPC pushConstant{};
            pushConstant.m_albedo = sceneMat.GetAlbedo();
            pushConstant.m_params = glm::vec4(sceneMat.GetMetallic(),
                sceneMat.GetRoughness(),
                sceneMat.GetAO(),
                0.0f);
            m_list.m_material.m_pushConstant = pushConstant;

#define UPDATE_TEXTURE(Name, member)                                                     \
            {                                                                                \
                auto& tex = sceneMat.Get##Name##Tex();                                       \
                if (tex.IsDirty())                                                           \
                {                                                                            \
                    tex.ClearDirty();                                                        \
                    DestroyRenderTexture(m_list.m_material.m_##member);                      \
                    m_list.m_material.m_##member = CreateRenderTexture(                      \
                        tex.GetPixels(), tex.GetWidth(), tex.GetHeight(), tex.GetType());    \
                    std::clog << "[Renderer] Upload " #Name " texture: "                     \
                              << tex.GetName() << ", " << tex.GetPixelCount() << " bytes\n"; \
                }                                                                            \
            }

            UPDATE_TEXTURE(Albedo, albedo);
            UPDATE_TEXTURE(Normal, normal);
            UPDATE_TEXTURE(Metallic, metallic);
            UPDATE_TEXTURE(Roughness, roughness);
            UPDATE_TEXTURE(AO, ao);

#undef UPDATE_TEXTURE
        }

        // Mesh
        {
            auto& sceneMesh = scene.GetMesh();
            bool hasNewMesh = sceneMesh.IsDirty();
            if (hasNewMesh)
            {
                sceneMesh.ClearDirty();

                // Destroy old mesh
                DestroyRenderMesh(m_list.m_mesh);

                // Create new mesh
                m_list.m_mesh = CreateRenderMesh(sceneMesh.GetVertices(), sceneMesh.GetIndices());

                std::clog << "[Renderer] Upload mesh: " << sceneMesh.GetName() << ", "
                    << sceneMesh.GetIndexCount() << " indices\n";
            }
        }
    }

    RenderList RenderScene::GetRenderList() const
    {
        return m_list;
    }
    RenderPerFrame RenderScene::CreateRenderPerFrame()
    {
        RenderPerFrame frame;

        frame.m_uboHandles.resize(kMaxFramesInFlight);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(FrameUbo);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        for (auto& handle : frame.m_uboHandles)
        {
            handle = m_resources.CreateBuffer(bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
        }

        return frame;
    }

    void RenderScene::DestroyRenderPerFrame(RenderPerFrame& frame)
    {
        for (auto& handle : frame.m_uboHandles)
        {
            m_resources.DestroyBuffer(handle);
        }
        frame = {};
    }

    RenderMaterial RenderScene::CreateRenderMaterial()
    {
        RenderMaterial material;

        // Textures fallback
        {
            uint8_t white[] = { 255, 255, 255, 255 };
            uint8_t flat[] = { 128, 128, 255, 255 };
            uint32_t width = 1;
            uint32_t height = 1;

            material.m_albedo = CreateRenderTexture(std::vector<uint8_t>(white, white + 4), width, height, Texture::Type::Albedo);
            material.m_normal = CreateRenderTexture(std::vector<uint8_t>(flat, flat + 4), width, height, Texture::Type::Normal);
            material.m_metallic = CreateRenderTexture(std::vector<uint8_t>(white, white + 1), width, height, Texture::Type::Linear);
            material.m_roughness = CreateRenderTexture(std::vector<uint8_t>(white, white + 1), width, height, Texture::Type::Linear);
            material.m_ao = CreateRenderTexture(std::vector<uint8_t>(white, white + 1), width, height, Texture::Type::Linear);
        }

        // Samplers
        {
            VkSamplerCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            // Todo: Enable Anisotropy
            createInfo.anisotropyEnable = VK_FALSE;
            createInfo.unnormalizedCoordinates = VK_FALSE;
            createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            createInfo.compareEnable = VK_FALSE;
            createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.mipLodBias = 0.0f;
            createInfo.minLod = 0.0f;
            createInfo.maxLod = VK_LOD_CLAMP_NONE;

            RenderSamplerHandle handle = m_resources.CreateSampler(createInfo);
            material.m_albedoSamplerHandle = handle;
            material.m_normalSamplerHandle = handle;
            material.m_metallicSamplerHandle = handle;
            material.m_roughnessSamplerHandle = handle;
            material.m_aoSamplerHandle = handle;
        }

        return material;
    }

    void RenderScene::DestroyRenderMaterial(RenderMaterial& material)
    {
        // Samplers(only destroy once)
        m_resources.DestroySampler(material.m_albedoSamplerHandle);

        material = {};
    }

    RenderMesh RenderScene::CreateRenderMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        assert(vertices.size() && "Vertices data is invalid");
        assert(indices.size() && "Indices data is invalid");

        // Vertex buffer
        RenderBufferHandle vertexHandle;
        {
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            vertexHandle = m_resources.CreateBufferWithData(bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                vertices.data(), static_cast<size_t>(bufferSize));
        }

        // Index buffer
        RenderBufferHandle indexHandle;
        {
            VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            indexHandle = m_resources.CreateBufferWithData(bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                indices.data(), static_cast<size_t>(bufferSize));
        }

        // Index count
        uint32_t indexCount = static_cast<uint32_t>(indices.size());

        return { vertexHandle, indexHandle, indexCount };
    }

    void RenderScene::DestroyRenderMesh(RenderMesh& mesh)
    {
        m_resources.DestroyBuffer(mesh.m_vertexBufferHandle);
        m_resources.DestroyBuffer(mesh.m_indexBufferHandle);

        mesh = {};
    }

    RenderTexture RenderScene::CreateRenderTexture(const std::vector<uint8_t>& pixels, uint32_t width, uint32_t height, Texture::Type type)
    {
        VkFormat format = TypeToFormat(type);
        auto mipLevels = static_cast<uint32_t>(
            std::floor(std::log2(std::max(width, height))) + 1
            );

        // Image
        RenderImageHandle imageHandle;
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            imageHandle = m_resources.CreateImageWithData(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                pixels.data(), pixels.size());
        }

        // Image view
        RenderImageViewHandle imageViewHandle;
        {
            RenderImage* image = m_resources.GetImage(imageHandle);
            assert(image && "Image handle is invalid");

            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = image->m_image;
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format = format;
            imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = mipLevels;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = 1;

            imageViewHandle = m_resources.CreateImageView(imageViewInfo);
        }

        return { imageHandle, imageViewHandle };
    }

    void RenderScene::DestroyRenderTexture(RenderTexture& texture)
    {
        m_resources.DestroyImageView(texture.m_imageViewHandle);
        m_resources.DestroyImage(texture.m_imageHandle);

        texture = {};
    }
}