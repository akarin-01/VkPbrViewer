#include "render_scene.h"

#include "scene/scene.h"
#include "scene/vertex.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"

#include <iostream>
#include <glm/glm.hpp>
#include <cassert>

namespace Kita::Pbrv
{
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
            MaterialUbo ubo{};
            ubo.m_albedo = sceneMat.GetAlbedo();
            ubo.m_params = glm::vec4(sceneMat.GetMetallic(), sceneMat.GetRoughness(), sceneMat.GetAO(), 0.0f);
            m_resources.WriteBuffer(m_list.m_material.m_uboHandles[frameIndex], &ubo, sizeof(ubo));
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
                    << m_list.m_mesh.m_indexCount << " indices\n";
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

        material.m_uboHandles.resize(kMaxFramesInFlight);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(MaterialUbo);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        for (auto& handle : material.m_uboHandles)
        {
            handle = m_resources.CreateBuffer(bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
        }

        return material;
    }

    void RenderScene::DestroyRenderMaterial(RenderMaterial& material)
    {
        for (auto& handle : material.m_uboHandles)
        {
            m_resources.DestroyBuffer(handle);
        }
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
}