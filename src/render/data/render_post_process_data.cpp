#include "render_post_process_data.h"

#include "scene/post_process.h"

#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"

#include <glm/glm.hpp>
#include <cassert>
#include <cmath>

namespace Kita::Pbrv
{
    RenderPostProcessData::RenderPostProcessData(const RenderContext& context,
        RenderResources& resources,
        const DescriptorAllocator& descriptorAllocator)
        : m_context(context),
        m_resources(resources),
        m_descriptorAllocator(descriptorAllocator)
    {
        // UBO
        {
            VkBufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.size = sizeof(PostProcessUbo);
            createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            for (auto& handle : m_uboHandles)
            {
                handle = m_resources.CreateBuffer(createInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
            }
        }

        // Set layout
        {
            std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_setLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Set
        for (size_t i = 0; i < m_sets.size(); ++i)
        {
            m_sets[i] = m_descriptorAllocator.Allocate(m_setLayout, "Post process set");

            VkDescriptorBufferInfo bufferInfo{};
            {
                RenderBuffer* buffer = m_resources.GetBuffer(m_uboHandles[i]);
                assert(buffer && "Post process buffer handle is invalid");
                bufferInfo.buffer = buffer->m_buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(PostProcessUbo);
            }

            std::array<VkWriteDescriptorSet, 1> writes{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = m_sets[i];
            writes[0].dstBinding = 0;
            writes[0].dstArrayElement = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[0].descriptorCount = 1;
            writes[0].pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(m_context.Device(),
                static_cast<uint32_t>(writes.size()), writes.data(),
                0, nullptr);
        }
    }

    RenderPostProcessData::~RenderPostProcessData()
    {
        // Sets will be released automatically

        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);

        for (auto& handle : m_uboHandles)
        {
            m_resources.DestroyBuffer(handle);
        }
    }

    void RenderPostProcessData::Update(uint32_t frameIndex, const PostProcess& postProcess)
    {
        PostProcessUbo ubo{};
        ubo.m_exposure = glm::vec4(std::exp2(postProcess.GetEV()), 0.0f, 0.0f, 0.0f);

        m_resources.WriteBuffer(m_uboHandles[frameIndex], &ubo, sizeof(ubo));
    }
}
