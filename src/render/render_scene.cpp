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
        m_camera = CreateRenderCamera();
    }


    RenderScene::~RenderScene()
    {
        DestroyRenderMesh(m_mesh);
        DestroyRenderCamera(m_camera);
    }

    void RenderScene::Update(const Scene& scene, const FrameInfo& frameInfo)
    {
        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& frameIndex = frameInfo.m_frameIndex;
        auto& imageIndex = frameInfo.m_imageIndex;

        // Camera
        {
            auto& sceneCamera = scene.GetCamera();
            FrameUbo ubo{};
            ubo.viewProj = sceneCamera.GetProjectMatrix(m_swapChain.Aspect())
                * sceneCamera.GetViewMatrix();
            m_resources.WriteBuffer(m_camera.m_viewProjUboHandles[frameIndex], &ubo, sizeof(ubo));
        }

        // Mesh
        {
            auto& sceneMesh = scene.GetMesh();
            bool hasNewMesh = sceneMesh.IsDirty();
            if (hasNewMesh)
            {
                sceneMesh.ClearDirty();

                // Destroy old mesh
                DestroyRenderMesh(m_mesh);

                // Create new mesh
                m_mesh = CreateRenderMesh(sceneMesh.GetVertices(), sceneMesh.GetIndices());

                std::clog << "[Renderer] Upload mesh: " << sceneMesh.GetName() << ", "
                    << m_mesh.m_indexCount << " indices\n";
            }
        }

        // Todo: Upload other gpu resource (e.g. camera, lights, materials, etc.)
    }

    RenderList RenderScene::GetRenderList() const
    {
        RenderList list{};
        list.m_camera = m_camera;
        list.m_mesh = m_mesh;
        return list;
    }

    RenderCamera RenderScene::CreateRenderCamera()
    {
        RenderCamera camera;

        camera.m_viewProjUboHandles.resize(kMaxFramesInFlight);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(FrameUbo);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        for (auto& handle : camera.m_viewProjUboHandles)
        {
            handle = m_resources.CreateBuffer(bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
        }

        return camera;
    }

    void RenderScene::DestroyRenderCamera(RenderCamera& camera)
    {
        for (auto& handle : camera.m_viewProjUboHandles)
        {
            m_resources.DestroyBuffer(handle);
        }
        camera = {};
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