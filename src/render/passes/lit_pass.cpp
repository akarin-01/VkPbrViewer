#include "lit_pass.h"

#include "rhi/context.h"
#include "rhi/graphics_pipeline.h"
#include "rhi/rendering_scope.h"
#include "rhi/swap_chain.h"
#include "render/vertex_input.h"
#include "render/render_scene.h"

#include <algorithm>
#include <cassert>
#include <unordered_map>

namespace Kita::Pbrv
{
    namespace Render
    {
        LitPass::LitPass(const Rhi::Context& context,
            const Rhi::SwapChain& swapChain,
            const RenderScene& scene)
            : RenderPassBase(context, swapChain),
            m_target(scene.GetTarget()),
            m_frame(scene.GetFrame()),
            m_lit(scene.GetLit()),
            m_objects(scene.GetObjects())
        {
            CreatePipeline(
                {
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::PerFrame),
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::Lit),
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::PerMaterial),
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::PerObject),
                });
        }

        LitPass::~LitPass() = default;

        void LitPass::RecreateResources()
        {
            /* Empty */
        }

        void LitPass::Draw(const Rhi::FrameInfo& frameInfo) const
        {
            assert(m_pipeline && "LitPass: pipeline is null");

            auto& commandBuffer = frameInfo.m_commandBuffer;
            auto& frameIndex = frameInfo.m_frameIndex;

            // Begin rendering
            {
                std::array<VkClearValue, 2> clearValues{};
                clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[1].depthStencil = { 1.0f, 0 };
                VkExtent2D extent = m_swapChain.Extent();

                Rhi::RenderingAttachmentDesc colorDesc{};
                colorDesc.m_imageView = m_target.GetColorImageView();
                colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorDesc.m_resolveImageView = m_target.GetResolveImageView();
                colorDesc.m_resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorDesc.m_clearValue = clearValues[0];

                Rhi::RenderingAttachmentDesc depthDesc{};
                depthDesc.m_imageView = m_target.GetDepthImageView();
                depthDesc.m_imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthDesc.m_clearValue = clearValues[1];

                Rhi::RenderingScope scope(commandBuffer, extent, { colorDesc }, &depthDesc);

                // Sort by material, then by mesh: minimal bind switches
                std::vector<const Render::RenderObject*> sortedObjects;
                sortedObjects.reserve(m_objects.size());
                for (auto& object : m_objects)
                {
                    sortedObjects.push_back(&object);
                }
                std::sort(sortedObjects.begin(), sortedObjects.end(),
                    [](const Render::RenderObject* a, const Render::RenderObject* b)
                    {
                        if (a->GetMaterialId() != b->GetMaterialId())
                        {
                            return a->GetMaterialId() < b->GetMaterialId();
                        }
                        return a->GetMeshId() < b->GetMeshId();
                    });

                // Bind pipeline
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

                // Draw
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    0, 1, &m_frame.GetSet(frameIndex), 0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    1, 1, &m_lit.GetSet(), 0, nullptr);

                Resource::ResourceId lastMaterialId{ Resource::kInvalidId };
                Resource::ResourceId lastMeshId{ Resource::kInvalidId };
                for (const auto* object : sortedObjects)
                {
                    if (!object->HasMesh())
                    {
                        continue;
                    }

                    if (object->GetMaterialId() != lastMaterialId)
                    {
                        lastMaterialId = object->GetMaterialId();

                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                            2, 1, &object->GetMaterialSet(), 0, nullptr);
                    }
                    if (object->GetMeshId() != lastMeshId)
                    {
                        lastMeshId = object->GetMeshId();

                        VkBuffer buffers[]{ object->GetVertexBuffer() };
                        VkDeviceSize offsets[]{ 0 };
                        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
                        vkCmdBindIndexBuffer(commandBuffer, object->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
                    }

                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                        3, 1, &object->GetObjectSet(frameIndex), 0, nullptr);

                    vkCmdDrawIndexed(commandBuffer, object->GetIndexCount(), 1, 0, 0, 0);
                }
            }
            // End rendering
        }

        void LitPass::CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts)
        {
            Rhi::GraphicsPipelineBuilder builder(m_context.Device());
            builder.SetShaders("assets/shaders/lit_vert.spv", "assets/shaders/lit_frag.spv")
                .SetVertexInput({ VertexInput::Binding() }, VertexInput::Attributes())
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetRasterizationSamples(m_context.SampleCount())
                .SetDepth(true, true, VK_COMPARE_OP_LESS)
                .SetDescriptorSetLayouts(layouts)
                .SetDynamicRendering({ m_target.GetColorFormat() }, m_target.GetDepthFormat());
            m_pipeline = builder.Build();
        }
    }
}
