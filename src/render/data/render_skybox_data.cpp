#include "render_skybox_data.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "scene/skybox.h"

#include <array>

namespace Kita::Pbrv
{
    RenderSkyboxData::RenderSkyboxData(const RenderContext& context,
        RenderResources& resources,
        const DescriptorAllocator& descriptorAllocator)
        : m_context(context),
        m_resources(resources),
        m_descriptorAllocator(descriptorAllocator)
    {
        // Set layout
        {
            std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_setLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Sets (contents are written once the cubemap is implemented)
        for (auto& set : m_sets)
        {
            set = m_descriptorAllocator.Allocate(m_setLayout, "Skybox set");
        }
    }

    RenderSkyboxData::~RenderSkyboxData()
    {
        // Sets will be destroyed automatically

        // Texture (only created once the cubemap is implemented; destroying zero handles is a safe no-op)
        m_resources.DestroySampler(m_texture.m_samplerHandle);
        m_resources.DestroyImageView(m_texture.m_imageViewHandle);
        m_resources.DestroyImage(m_texture.m_imageHandle);

        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);
    }

    void RenderSkyboxData::Update(uint32_t frameIndex, const Skybox& sceneSkybox)
    {
        if (sceneSkybox.IsDirty())
        {
            sceneSkybox.ClearDirty();

            // TODO: equirect -> cubemap conversion + texture upload (RenderTexture)
            // For now only the descriptor set refresh is flagged; WriteSet is implemented once the cubemap is ready.

            m_setRefreshCount = kMaxFramesInFlight;

            KITA_LOG_DEBUG("[Renderer] Update skybox descriptor set: textures changed");
        }

        bool setRefreshed = m_setRefreshCount > 0;
        if (setRefreshed)
        {
            WriteSet(m_sets[frameIndex]);
            --m_setRefreshCount;
        }
    }

    void RenderSkyboxData::WriteSet(VkDescriptorSet /*set*/)
    {
        // TODO: bind the cubemap's imageView + sampler
        // (to be implemented together with the equirect -> cubemap conversion)
    }
}
