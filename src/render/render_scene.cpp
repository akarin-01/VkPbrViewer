#include "render_scene.h"
#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_manager.h"
#include "resource/resource_manager.h"
#include "scene/scene.h"

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Render
    {
        RenderScene::RenderScene(const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            Resource::DescriptorManager& descriptorMgr,
            Resource::ResourceManager& resourceMgr)
            : m_context(context),
            m_resourceMgr(resourceMgr),
            m_descriptorMgr(descriptorMgr),
            m_target(context, resources, swapChain.Extent()),
            m_frameData(context, resources, swapChain, descriptorMgr),
            m_materialData(context, resources, descriptorMgr),
            m_postProcessData(context, resources, descriptorMgr, m_target.GetResolveTexture())
        {
            m_objectState = CreateObjectState();
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Scene::Scene& scene, const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;

            UpdateObjectState(frameIndex, scene.GetObject());

            m_frameData.UpdateUbo(frameIndex, scene.GetCamera(), scene.GetLight());
            m_frameData.UpdateSkybox(scene.GetSkybox());
            m_frameData.RefreshSet(frameIndex);

            m_materialData.UpdateTextures(scene.GetObject().GetMaterial());
            m_materialData.RefreshSet(frameIndex);

            m_postProcessData.UpdateUbo(frameIndex, scene.GetPostProcess());
            m_postProcessData.RefreshSet(frameIndex);
        }

        void RenderScene::Recreate(VkExtent2D extent)
        {
            m_target.Recreate(extent);
            m_postProcessData.UpdateTarget(m_target.GetResolveTexture());
        }

        VkDescriptorSetLayout RenderScene::GetEmptyLayout() const
        {
            return m_descriptorMgr.GetLayout(Resource::DescriptorLayoutType::Empty);
        }

        ObjectState RenderScene::CreateObjectState() const
        {
            constexpr Resource::DescriptorLayoutType kLayoutType =
                Resource::DescriptorLayoutType::PerObject;

            ObjectState object{};
            object.m_layout = m_descriptorMgr.GetLayout(kLayoutType);

            Resource::BufferDesc desc{};
            desc.m_size = sizeof(PerObjectUbo);
            desc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.m_properties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            desc.m_mapped = true;

            for (size_t i = 0; i < object.m_ubos.size(); ++i)
            {
                object.m_ubos[i] = m_resourceMgr.CreateBuffer(desc);
                object.m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                Rhi::V2::DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    object.m_ubos[i]->m_buffer, 0, sizeof(PerObjectUbo))
                    .UpdateSet(object.m_sets[i]);
            }

            return object;
        }

        void RenderScene::UpdateObjectState(uint32_t frameIndex, const Scene::Object& object)
        {
            const Resource::ResourceId meshId = object.GetMesh().GetId();
            if (meshId != m_objectState.m_lastMeshId)
            {
                m_objectState.m_lastMeshId = meshId;
                m_objectState.m_mesh = m_resourceMgr.GetOrCreateMesh(meshId);
            }

            auto& mat = object.GetMaterial();
            PerObjectUbo ubo{};
            ubo.m_transform.m_model = glm::mat4(1.0f);
            ubo.m_transform.m_normal = glm::mat4(1.0f);
            ubo.m_material.m_albedo = mat.GetAlbedo();
            ubo.m_material.m_pbrParams = glm::vec4(mat.GetMetallic(), mat.GetRoughness(), mat.GetAO(), 0.0f);
            ubo.m_material.m_emissive = glm::vec4(mat.GetEmissive(), 1.0f);
            m_objectState.WriteUbo(frameIndex, ubo);
        }
    }
}
