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
            m_target(scene.GetGlobal().m_target),
            m_frameSet(scene.GetGlobal().m_frameSet),
            m_litSet(scene.GetGlobal().m_litSet),
            m_objectMap(scene.GetObjects())
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
                std::vector<const Render::ObjectState*> sortedObjects;
                sortedObjects.reserve(m_objectMap.size());
                for (auto& [id, objectState] : m_objectMap)
                {
                    sortedObjects.push_back(&objectState);
                }
                std::sort(sortedObjects.begin(), sortedObjects.end(),
                    [](const Render::ObjectState* a, const Render::ObjectState* b)
                    {
                        const Resource::ResourceId aMaterial = a->m_materialSet.GetId();
                        const Resource::ResourceId bMaterial = b->m_materialSet.GetId();
                        if (aMaterial != bMaterial)
                        {
                            return aMaterial < bMaterial;
                        }
                        return a->m_mesh.GetId() < b->m_mesh.GetId();
                    });

                // Bind pipeline
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

                // Draw
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    0, 1, &m_frameSet.GetSet(frameIndex), 0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    1, 1, &m_litSet.GetSet(), 0, nullptr);

                Resource::ResourceId lastMaterialId{ Resource::kInvalidId };
                Resource::ResourceId lastMeshId{ Resource::kInvalidId };
                for (const auto* objectState : sortedObjects)
                {
                    if (!objectState->m_mesh)
                    {
                        continue;
                    }

                    const Resource::ResourceId materialId = objectState->m_materialSet.GetId();
                    if (materialId != lastMaterialId)
                    {
                        lastMaterialId = materialId;

                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                            2, 1, &objectState->m_materialSet->GetSet(), 0, nullptr);
                    }
                    const Resource::ResourceId meshId = objectState->m_mesh.GetId();
                    if (meshId != lastMeshId)
                    {
                        lastMeshId = meshId;

                        VkBuffer buffers[]{ objectState->m_mesh->GetVertexBuffer() };
                        VkDeviceSize offsets[]{ 0 };
                        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
                        vkCmdBindIndexBuffer(commandBuffer, objectState->m_mesh->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
                    }

                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                        3, 1, &objectState->m_objectSet.GetSet(frameIndex), 0, nullptr);

                    vkCmdDrawIndexed(commandBuffer, objectState->m_mesh->GetIndexCount(), 1, 0, 0, 0);
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
