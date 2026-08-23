#include "compute_conversion.h"

#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/descriptor_allocator.h"
#include "render/compute_pipeline.h"
#include "render/descriptor_writer.h"
#include "render/one_shot_command.h"

#include <cassert>

namespace Kita::Pbrv
{
    ComputeConversion::ComputeConversion(const RenderContext& context,
        RenderResources& resources,
        const DescriptorAllocator& descriptorAllocator,
        uint32_t inputCount,
        const std::string& shaderPath, uint32_t pushConstantSize)
        : m_context(context),
        m_resources(resources),
        m_descriptorAllocator(descriptorAllocator),
        m_pushConstantSize(pushConstantSize)
    {
        // Set layout
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings(inputCount + 1);
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            for (uint32_t i = 1; i <= inputCount; ++i)
            {
                bindings[i].binding = i;
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                bindings[i].descriptorCount = 1;
                bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            }

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            m_setLayout = CreateDescriptorSetLayout(m_context.Device(), createInfo);
        }

        // Set
        {
            m_set = m_descriptorAllocator.Allocate(m_setLayout, "Conversion set");
        }

        // Pipeline
        {
            ComputePipelineBuilder builder(m_context.Device());
            builder.SetShader(shaderPath)
                .SetDescriptorSetLayouts({ m_setLayout });

            if (pushConstantSize > 0)
            {
                VkPushConstantRange pushConstant{};
                pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                pushConstant.offset = 0;
                pushConstant.size = pushConstantSize;

                builder.SetPushConstants({ pushConstant });
            }

            m_pipeline = builder.Build();
        }
    }

    ComputeConversion::~ComputeConversion()
    {
        m_pipeline.reset();

        // Sets will be destroyed automatically

        vkDestroyDescriptorSetLayout(m_context.Device(), m_setLayout, nullptr);
    }

    void ComputeConversion::Dispatch(const Output& output, VkExtent3D dispatchSize, const std::vector<Input>& inputs, const void* pushData) const
    {
        assert(m_pipeline && "ComputeConversion: pipeline is null");

        RenderImage* outputImage = m_resources.GetImage(output.m_image);
        assert(outputImage && "ComputeConversion: output image handle is invalid");

        // 1. Write set
        {
            DescriptorWriter writer(m_resources, m_context.Device());
            writer.WriteImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                VK_IMAGE_LAYOUT_GENERAL, output.m_imageView, 0);
            for (size_t i = 0; i < inputs.size(); ++i)
            {
                auto& input = inputs[i];
                if (input.m_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                    writer.WriteImage(static_cast<uint32_t>(i + 1), input.m_type,
                        input.m_layout, input.m_imageView, input.m_sampler);
                }
                // Other type...
            }
            writer.UpdateSet(m_set);
        }

        // 2. Dispatch conversion
        {
            OneShotCommand command(m_context);

            // Output: UNDEFINED -> GENERAL
            TransitionImageLayout(command.Handle(), outputImage->m_image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                output.m_range);

            if (m_pushConstantSize > 0)
            {
                assert(pushData && "ComputeConversion: push data is null");
                vkCmdPushConstants(command.Handle(), m_pipeline->Layout(), VK_SHADER_STAGE_COMPUTE_BIT,
                    0, m_pushConstantSize, pushData);
            }
            vkCmdBindPipeline(command.Handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->Handle());
            vkCmdBindDescriptorSets(command.Handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->Layout(),
                0, 1, &m_set, 0, nullptr);
            vkCmdDispatch(command.Handle(), dispatchSize.width, dispatchSize.height, dispatchSize.depth);

            // Output: GENERAL -> final layout
            TransitionImageLayout(command.Handle(), outputImage->m_image,
                VK_IMAGE_LAYOUT_GENERAL, output.m_finalLayout,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                output.m_finalStage, output.m_finalAccess,
                output.m_range);
        }
    }
}
