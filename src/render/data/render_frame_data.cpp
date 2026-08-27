#include "render_frame_data.h"
#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_manager.h"
#include "resource/resources.h"
#include "scene/camera.h"
#include "scene/light.h"

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr Resource::DescriptorLayoutType kLayoutType = Resource::DescriptorLayoutType::Frame;
        }

        RenderFrameData::RenderFrameData(const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            Resource::DescriptorManager& descriptorMgr)
            : m_context(context),
            m_resources(resources),
            m_swapChain(swapChain),
            m_descriptorMgr(descriptorMgr)
        {
            // UBO
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

            // Set
            for (size_t i = 0; i < m_sets.size(); ++i)
            {
                m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                Rhi::DescriptorWriter writer(m_resources, m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    m_uboHandles[i], 0, sizeof(FrameUbo))
                    .UpdateSet(m_sets[i]);
            }
        }

        RenderFrameData::~RenderFrameData()
        {
            // Sets will be released automatically

            for (auto& handle : m_uboHandles)
            {
                m_resources.DestroyBuffer(handle);
            }
        }

        void RenderFrameData::Update(uint32_t frameIndex, const Scene::Camera& camera, const Scene::Light& light)
        {
            FrameUbo ubo{};
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 proj = camera.GetProjectMatrix(m_swapChain.Aspect());
            ubo.m_viewProj = proj * view;
            ubo.m_skyboxViewProj = proj * glm::mat4(glm::mat3(view));
            ubo.m_viewPos = glm::vec4(camera.GetPosition(), 1.0f);
            ubo.m_lightPos = glm::vec4(light.GetPosition(), 0.0f);
            ubo.m_lightColor = glm::vec4(light.GetColor(), light.GetIntensity());

            m_resources.WriteBuffer(m_uboHandles[frameIndex], &ubo, sizeof(ubo));
        }

        VkDescriptorSetLayout RenderFrameData::GetSetLayout() const
        {
            return m_descriptorMgr.GetLayout(kLayoutType);
        }
    }
}
