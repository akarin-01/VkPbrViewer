#include "render_scene.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/swap_chain.h"
#include "scene/scene.h"
#include "scene/vertex.h"
#include "scene/texture.h"

#include <glm/glm.hpp>
#include <cassert>
#include <stdexcept>
#include <algorithm>

namespace Kita::Pbrv
{
    namespace
    {
        VkFormat ToFormat(TextureType type)
        {
            switch (type)
            {
            case TextureType::Albedo:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureType::Normal:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureType::Linear:
                return VK_FORMAT_R8_UNORM;
            default:
                throw std::runtime_error("Invalid texture type!");
            }
        }

        const Texture& GetTexture(const Material& mat, uint32_t slot)
        {
            switch (slot)
            {
            case Albedo:
                return mat.GetAlbedoTex();
            case Normal:
                return mat.GetNormalTex();
            case Metallic:
                return mat.GetMetallicTex();
            case Roughness:
                return mat.GetRoughnessTex();
            case AO:
                return mat.GetAOTex();
            default:
                throw std::runtime_error("Invalid texture slot!");
            }
        }
    }

    RenderScene::RenderScene(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator)
        : m_context(context),
        m_resources(resources),
        m_swapChain(swapChain),
        m_descriptorAllocator(descriptorAllocator)
    {
        CreateFallbackTextures();

        m_list.m_frame = CreateRenderPerFrame();
        // RenderMesh would be created when update
        m_list.m_material = CreateRenderMaterial();
    }

    RenderScene::~RenderScene()
    {
        DestroyRenderMaterial(m_list.m_material);
        DestroyRenderMesh(m_list.m_mesh);
        DestroyRenderPerFrame(m_list.m_frame);
        DestroyFallbackTextures();
    }

    void RenderScene::Update(const Scene& scene, const FrameInfo& frameInfo)
    {
        auto& frameIndex = frameInfo.m_frameIndex;

        UpdateRenderPerFrame(m_list.m_frame, frameIndex, scene.GetCamera(), scene.GetLight());
        UpdateRenderMaterial(m_list.m_material, frameIndex, scene.GetMaterial());
        UpdateRenderMesh(m_list.m_mesh, scene.GetMesh());
        UpdateRenderSkybox(m_list.m_skybox, frameIndex, scene.GetSkybox());
    }

    const RenderList& RenderScene::GetRenderList() const
    {
        return m_list;
    }

    void RenderScene::CreateFallbackTextures()
    {
        uint8_t white[] = { 255, 255, 255, 255 };
        uint8_t flat[] = { 128, 128, 255, 255 };
        uint32_t width = 1;
        uint32_t height = 1;

        // Albedo
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(white, white + 4), width, height, TextureType::Albedo);
            m_fallbackTextures[Albedo] = CreateRenderTexture(tex);
        }
        // Normal
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(flat, flat + 4), width, height, TextureType::Normal);
            m_fallbackTextures[Normal] = CreateRenderTexture(tex);
        }
        // Linear
        {
            Texture tex{};
            tex.SetData("fallback", std::vector<uint8_t>(white, white + 1), width, height, TextureType::Linear);
            RenderTexture linearFallback = CreateRenderTexture(tex);
            m_fallbackTextures[Metallic] = linearFallback;
            m_fallbackTextures[Roughness] = linearFallback;
            m_fallbackTextures[AO] = linearFallback;
        }
    }

    void RenderScene::DestroyFallbackTextures()
    {
        for (auto& texture : m_fallbackTextures)
        {
            DestroyRenderTexture(texture);
        }
    }

    RenderPerFrame RenderScene::CreateRenderPerFrame() const
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

    void RenderScene::UpdateRenderPerFrame(RenderPerFrame& frame, uint32_t frameIndex, const Camera& sceneCamera, const Light& sceneLight) const
    {
        FrameUbo ubo{};
        glm::mat4 view = sceneCamera.GetViewMatrix();
        glm::mat4 proj = sceneCamera.GetProjectMatrix(m_swapChain.Aspect());
        ubo.m_viewProj = proj * view;
        ubo.m_skyboxViewProj = proj * glm::mat4(glm::mat3(view));
        ubo.m_viewPos = glm::vec4(sceneCamera.GetPosition(), 1.0f);
        ubo.m_lightDir = glm::vec4(sceneLight.GetDirection(), 0.0f);
        ubo.m_lightColor = glm::vec4(sceneLight.GetColor(), sceneLight.GetIntensity());

        m_resources.WriteBuffer(frame.m_uboHandles[frameIndex], &ubo, sizeof(ubo));
    }

    RenderMaterial RenderScene::CreateRenderMaterial() const
    {
        RenderMaterial material{};

        // Textures
        material.m_textures = m_fallbackTextures;

        // Set layout
        {
            std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[0].descriptorCount = kMaterialTextureCount;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            material.m_setLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Sets
        {
            for (auto& set : material.m_sets)
            {
                set = m_descriptorAllocator.Allocate(material.m_setLayout, "Material set");
                WriteMaterialSet(set, material.m_textures);
            }
        }

        return material;
    }

    void RenderScene::DestroyRenderMaterial(RenderMaterial& material)
    {
        // Sets will be destroyed automatically

        // Set layout
        vkDestroyDescriptorSetLayout(m_context.Device(), material.m_setLayout, nullptr);

        // Textures
        {
            auto& textures = material.m_textures;
            for (size_t i = 0; i < textures.size(); ++i)
            {
                bool isFallback = (textures[i] == m_fallbackTextures[i]);
                if (!isFallback)
                {
                    DestroyRenderTexture(textures[i]);
                }
            }
        }

        material = {};
    }

    void RenderScene::UpdateRenderMaterial(RenderMaterial& material, uint32_t frameIndex, const Material& sceneMat)
    {
        MaterialPC pushConstant{};
        pushConstant.m_albedo = sceneMat.GetAlbedo();
        pushConstant.m_params = glm::vec4(sceneMat.GetMetallic(),
            sceneMat.GetRoughness(),
            sceneMat.GetAO(),
            0.0f);
        material.m_pushConstant = pushConstant;

        bool anyTexUpdated = false;
        for (uint32_t i = 0; i < kMaterialTextureCount; ++i)
        {
            anyTexUpdated |= UpdateRenderTexture(material.m_textures[i], i, GetTexture(sceneMat, i));
        }

        if (anyTexUpdated)
        {
            material.m_setRefreshCount = kMaxFramesInFlight;

            KITA_LOG_DEBUG("[Renderer] Update material descriptor set: textures changed");
        }

        bool setRefreshed = material.m_setRefreshCount > 0;
        if (setRefreshed)
        {
            WriteMaterialSet(material.m_sets[frameIndex], material.m_textures);
            --material.m_setRefreshCount;
        }
    }

    void RenderScene::WriteMaterialSet(VkDescriptorSet set, const std::array<RenderTexture, kMaterialTextureCount>& textures) const
    {
        std::array<VkDescriptorImageInfo, kMaterialTextureCount> imageInfos{};
        for (uint32_t i = 0; i < kMaterialTextureCount; ++i)
        {
            auto& texture = textures[i];

            RenderImageView* imageView = m_resources.GetImageView(texture.m_imageViewHandle);
            assert(imageView && "Image view handle is invalid");
            RenderSampler* sampler = m_resources.GetSampler(texture.m_samplerHandle);
            assert(sampler && "Sampler handle is invalid");

            auto& imageInfo = imageInfos[i];
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = imageView->m_imageView;
            imageInfo.sampler = sampler->m_sampler;
        }

        std::array<VkWriteDescriptorSet, 1> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = set;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = static_cast<uint32_t>(imageInfos.size());
        writes[0].pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(m_context.Device(),
            static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    RenderMesh RenderScene::CreateRenderMesh(const Mesh& sceneMesh) const
    {
        assert(!sceneMesh.IsEmpty() && "CreateRenderMesh requires non-empty mesh");

        auto& vertices = sceneMesh.GetVertices();
        auto& indices = sceneMesh.GetIndices();

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

    void RenderScene::DestroyRenderMesh(RenderMesh& mesh) const
    {
        m_resources.DestroyBuffer(mesh.m_vertexBufferHandle);
        m_resources.DestroyBuffer(mesh.m_indexBufferHandle);

        mesh = {};
    }

    bool RenderScene::UpdateRenderMesh(RenderMesh& mesh, const Mesh& sceneMesh) const
    {
        if (sceneMesh.IsDirty())
        {
            sceneMesh.ClearDirty();

            // Destroy old mesh
            DestroyRenderMesh(mesh);

            if (!sceneMesh.IsEmpty())
            {
                // Create new mesh
                mesh = CreateRenderMesh(sceneMesh);

                Log::Info("[Renderer] Upload mesh: ", sceneMesh.GetName(), ", ",
                    sceneMesh.GetIndexCount(), " indices");
            }

            return true;
        }
        return false;
    }

    RenderTexture RenderScene::CreateRenderTexture(const Texture& sceneTex) const
    {
        auto& pixels = sceneTex.GetPixels();
        auto width = sceneTex.GetWidth();
        auto height = sceneTex.GetHeight();
        auto type = sceneTex.GetType();

        VkFormat format = ToFormat(type);
        auto mipLevels = static_cast<uint32_t>(
            std::floor(std::log2(std::max(width, height))) + 1
            );

        // Image
        RenderImageHandle imageHandle{ 0 };
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
        RenderImageViewHandle imageViewHandle{ 0 };
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

        // Sampler
        RenderSamplerHandle samplerHandle = m_resources.CreateSamplerLinearRepeatMip();

        return { imageHandle, imageViewHandle, samplerHandle };
    }

    void RenderScene::DestroyRenderTexture(RenderTexture& texture) const
    {
        m_resources.DestroySampler(texture.m_samplerHandle);
        m_resources.DestroyImageView(texture.m_imageViewHandle);
        m_resources.DestroyImage(texture.m_imageHandle);

        texture = {};
    }

    bool RenderScene::UpdateRenderTexture(RenderTexture& texture, uint32_t slot, const Texture& sceneTex) const
    {
        if (sceneTex.IsDirty())
        {
            sceneTex.ClearDirty();

            bool isOldFallback = texture == m_fallbackTextures[slot];
            if (!isOldFallback)
            {
                DestroyRenderTexture(texture);
            }

            RenderTexture newTex{};
            if (sceneTex.IsEmpty())
            {
                // New texture is empty, use fallback
                newTex = m_fallbackTextures[slot];
            }
            else
            {
                // New texture is not empty, create new texture
                newTex = CreateRenderTexture(sceneTex);

                Log::Info("[Renderer] Upload ", ToString(MaterialTextureSlot(slot)), " texture: ",
                    sceneTex.GetName(), ", ", sceneTex.GetPixelCount(), " bytes");
            }
            texture = newTex;

            return true;
        }

        return false;
    }

    RenderSkybox RenderScene::CreateRenderSkybox(const Skybox& sceneSkybox) const
    {
        RenderSkybox skybox{};

        // Textures
        // m_resources.CreateCubemapFromEquirectData

        // Set layout
        {
            std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            skybox.m_setLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Sets
        {
            for (auto& set : skybox.m_sets)
            {
                set = m_descriptorAllocator.Allocate(skybox.m_setLayout, "Skybox set");
                WriteSkyboxSet(set, skybox.m_texture);
            }
        }

        return skybox;
    }

    void RenderScene::DestroyRenderSkybox(RenderSkybox& skybox) const
    {
        // Sets will be destroyed automatically

        // Set layout
        vkDestroyDescriptorSetLayout(m_context.Device(), skybox.m_setLayout, nullptr);

        // Textures
        DestroyRenderTexture(skybox.m_texture);

        skybox = {};
    }

    bool RenderScene::UpdateRenderSkybox(RenderSkybox& skybox, uint32_t frameIndex, const Skybox& sceneSkybox) const
    {
        if (sceneSkybox.IsDirty())
        {
            sceneSkybox.ClearDirty();

            // Update texture

            skybox.m_setRefreshCount = kMaxFramesInFlight;

            KITA_LOG_DEBUG("[Renderer] Update skybox descriptor set: textures changed");

            return true;
        }

        bool setRefreshed = skybox.m_setRefreshCount > 0;
        if (setRefreshed)
        {
            WriteSkyboxSet(skybox.m_sets[frameIndex], skybox.m_texture);
            --skybox.m_setRefreshCount;
        }

        return false;
    }

    void RenderScene::WriteSkyboxSet(VkDescriptorSet set, const RenderTexture& texture) const
    {
    }
}