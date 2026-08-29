#include "render_object_data.h"

#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "resource/descriptor_manager.h"
#include "resource/resources.h"
#include "scene/object.h"

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr Resource::DescriptorLayoutType kLayoutType = Resource::DescriptorLayoutType::PerObject;
        }

        RenderObjectData::RenderObjectData(const Rhi::Context& context,
            Resource::Resources& resources,
            Resource::DescriptorManager& descriptorMgr)
            : m_context(context),
            m_resources(resources),
            m_descriptorMgr(descriptorMgr)
        {
            VkBufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.size = sizeof(ObjectUbo);
            createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            for (auto& handle : m_uboHandles)
            {
                handle = m_resources.CreateBuffer(createInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
            }

            for (size_t i = 0; i < m_sets.size(); ++i)
            {
                m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                WriteSet(static_cast<uint32_t>(i));
            }
        }

        RenderObjectData::~RenderObjectData()
        {
            for (auto& handle : m_uboHandles)
            {
                m_resources.DestroyBuffer(handle);
            }
        }

        void RenderObjectData::UpdateUbo(uint32_t frameIndex, const Scene::Object& object)
        {
            ObjectUbo ubo{};
            ubo.m_transform.m_model = glm::mat4(1.0f);
            ubo.m_transform.m_normal = glm::mat4(1.0f);
            ubo.m_material.m_albedo = object.GetMaterial().GetAlbedo();
            ubo.m_material.m_pbrParams = glm::vec4(object.GetMaterial().GetMetallic(),
                object.GetMaterial().GetRoughness(), object.GetMaterial().GetAO(), 0.0f);
            ubo.m_material.m_emissive = glm::vec4(object.GetMaterial().GetEmissive(), 1.0f);

            m_resources.WriteBuffer(m_uboHandles[frameIndex], &ubo, sizeof(ubo));
        }

        VkDescriptorSetLayout RenderObjectData::GetSetLayout() const
        {
            return m_descriptorMgr.GetLayout(kLayoutType);
        }

        void RenderObjectData::WriteSet(uint32_t frameIndex) const
        {
            Rhi::DescriptorWriter writer(m_resources, m_context.Device());
            writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                m_uboHandles[frameIndex], 0, sizeof(ObjectUbo));

            writer.UpdateSet(m_sets[frameIndex]);
        }
    }
}
