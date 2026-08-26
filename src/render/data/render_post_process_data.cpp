#include "render_post_process_data.h"
#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "rhi/utils.h"
#include "resource/descriptor_allocator.h"
#include "resource/resources.h"
#include "scene/post_process.h"

#include <glm/glm.hpp>
#include <cmath>

namespace Kita::Pbrv
{
    namespace Render
    {
        RenderPostProcessData::RenderPostProcessData(const Rhi::RenderContext& context,
            Resource::RenderResources& resources,
            const Resource::DescriptorAllocator& descriptorAllocator)
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

                m_setLayout = Rhi::CreateDescriptorSetLayout(m_context.Device(), createInfo);
            }

            // Set
            for (size_t i = 0; i < m_sets.size(); ++i)
            {
                m_sets[i] = m_descriptorAllocator.Allocate(m_setLayout, "Post process set");

                Rhi::DescriptorWriter writer(m_resources, m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    m_uboHandles[i], 0, sizeof(PostProcessUbo))
                    .UpdateSet(m_sets[i]);
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

        void RenderPostProcessData::Update(uint32_t frameIndex, const Scene::PostProcess& postProcess)
        {
            PostProcessUbo ubo{};
            ubo.m_exposure = glm::vec4(std::exp2(postProcess.GetEV()), 0.0f, 0.0f, 0.0f);

            m_resources.WriteBuffer(m_uboHandles[frameIndex], &ubo, sizeof(ubo));
        }
    }
}
