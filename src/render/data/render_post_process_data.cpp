#include "render_post_process_data.h"
#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "resource/descriptor_manager.h"
#include "resource/resources.h"
#include "scene/post_process.h"

#include <glm/glm.hpp>
#include <cmath>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr Resource::DescriptorLayoutType kLayoutType = Resource::DescriptorLayoutType::PostProcess;
        }

        RenderPostProcessData::RenderPostProcessData(const Rhi::Context& context,
            Resource::Resources& resources,
            Resource::DescriptorManager& descriptorMgr)
            : m_context(context),
            m_resources(resources),
            m_descriptorMgr(descriptorMgr)
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

            // Set
            for (size_t i = 0; i < m_sets.size(); ++i)
            {
                m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                Rhi::DescriptorWriter writer(m_resources, m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    m_uboHandles[i], 0, sizeof(PostProcessUbo))
                    .UpdateSet(m_sets[i]);
            }
        }

        RenderPostProcessData::~RenderPostProcessData()
        {
            // Sets will be released automatically

            for (auto& handle : m_uboHandles)
            {
                m_resources.DestroyBuffer(handle);
            }
        }

        VkDescriptorSetLayout RenderPostProcessData::GetSetLayout() const
        {
            return m_descriptorMgr.GetLayout(Resource::DescriptorLayoutType::PostProcess);
        }

        void RenderPostProcessData::Update(uint32_t frameIndex, const Scene::PostProcess& postProcess)
        {
            PostProcessUbo ubo{};
            ubo.m_exposure = glm::vec4(std::exp2(postProcess.GetEV()), 0.0f, 0.0f, 0.0f);

            m_resources.WriteBuffer(m_uboHandles[frameIndex], &ubo, sizeof(ubo));
        }
    }
}
