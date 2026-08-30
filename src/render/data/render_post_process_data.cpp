#include "render_post_process_data.h"
#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "resource/descriptor_manager.h"
#include "resource/gpu_layouts.h"
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
            Resource::DescriptorManager& descriptorMgr,
            const Resource::RenderTexture& target)
            : m_context(context),
            m_resources(resources),
            m_descriptorMgr(descriptorMgr)
        {
            // UBO
            {
                VkBufferCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                createInfo.size = sizeof(Gpu::PostProcess);
                createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                for (auto& handle : m_uboHandles)
                {
                    handle = m_resources.CreateBuffer(createInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
                }
            }

            // Texture
            m_offlineTex = target;

            // Set
            for (size_t i = 0; i < m_sets.size(); ++i)
            {
                m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                WriteSet(static_cast<uint32_t>(i), true);
            }
        }

        RenderPostProcessData::~RenderPostProcessData()
        {
            for (auto& handle : m_uboHandles)
            {
                m_resources.DestroyBuffer(handle);
            }
        }

        void RenderPostProcessData::UpdateUbo(uint32_t frameIndex, const Scene::PostProcess& postProcess)
        {
            Gpu::PostProcess ubo{};
            ubo.m_exposure = glm::vec4(std::exp2(postProcess.GetEV()), 0.0f, 0.0f, 0.0f);

            m_resources.WriteBuffer(m_uboHandles[frameIndex], &ubo, sizeof(ubo));
        }

        void RenderPostProcessData::UpdateTarget(const Resource::RenderTexture& target)
        {
            if (m_offlineTex != target)
            {
                m_offlineTex = target;
                m_setDirtyCount = Rhi::kMaxFramesInFlight;
            }
        }

        void RenderPostProcessData::RefreshSet(uint32_t frameIndex)
        {
            if (m_setDirtyCount > 0)
            {
                WriteSet(frameIndex, false);
                --m_setDirtyCount;
            }
        }

        VkDescriptorSetLayout RenderPostProcessData::GetSetLayout() const
        {
            return m_descriptorMgr.GetLayout(kLayoutType);
        }

        void RenderPostProcessData::WriteSet(uint32_t frameIndex, bool writeUbo) const
        {
            Rhi::DescriptorWriter writer(m_resources, m_context.Device());
            if (writeUbo)
            {
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    m_uboHandles[frameIndex], 0, sizeof(Gpu::PostProcess));
            }

            writer.WriteImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_offlineTex.m_imageViewHandle, m_offlineTex.m_samplerHandle)
                .UpdateSet(m_sets[frameIndex]);
        }
    }
}
