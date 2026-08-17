#include "render_frame_data.h"

#include "scene/camera.h"
#include "scene/light.h"

#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"
#include "render/descriptor_allocator.h"

#include <glm/glm.hpp>
#include <cassert>

namespace Kita::Pbrv
{
    RenderFrameData::RenderFrameData(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator)
        : m_context(context),
        m_resources(resources),
        m_swapChain(swapChain),
        m_descriptorAllocator(descriptorAllocator)
    {
        // Ubo
        {
            VkBufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.size = sizeof(FrameUbo);
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
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_setLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Set
        for (size_t i = 0; i < m_sets.size(); ++i)
        {
            m_sets[i] = m_descriptorAllocator.Allocate(m_setLayout, "Frame set");

            VkDescriptorBufferInfo bufferInfo{};
            {
                RenderBuffer* buffer = m_resources.GetBuffer(m_uboHandles[i]);
                assert(buffer && "Frame buffer handle is invalid");
                bufferInfo.buffer = buffer->m_buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(FrameUbo);
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

    RenderFrameData::~RenderFrameData()
    {
        // Sets will be released automatically

        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);

        for (auto& handle : m_uboHandles)
        {
            m_resources.DestroyBuffer(handle);
        }
    }

    void RenderFrameData::Update(uint32_t frameIndex, const Camera& camera, const Light& light)
    {
        FrameUbo ubo{};
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = camera.GetProjectMatrix(m_swapChain.Aspect());
        ubo.m_viewProj = proj * view;
        ubo.m_skyboxViewProj = proj * glm::mat4(glm::mat3(view));
        ubo.m_viewPos = glm::vec4(camera.GetPosition(), 1.0f);
        ubo.m_lightDir = glm::vec4(light.GetDirection(), 0.0f);
        ubo.m_lightColor = glm::vec4(light.GetColor(), light.GetIntensity());

        m_resources.WriteBuffer(m_uboHandles[frameIndex], &ubo, sizeof(ubo));
    }
}