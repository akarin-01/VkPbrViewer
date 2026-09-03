#include "render_scene.h"

#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/descriptor_writer.h"
#include "resource/resource_manager.h"
#include "render/scene_proxy.h"

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr uint32_t kShadowMapSize = 2048;

            constexpr Resource::DescriptorSetRhi::Type kFrameType = Resource::DescriptorSetRhi::Type::PerFrame;
            constexpr Resource::DescriptorSetRhi::Type kPostProcessType = Resource::DescriptorSetRhi::Type::PostProcess;
            constexpr Resource::DescriptorSetRhi::Type kLitType = Resource::DescriptorSetRhi::Type::Lit;
            constexpr Resource::DescriptorSetRhi::Type kMaterialType = Resource::DescriptorSetRhi::Type::PerMaterial;
            constexpr Resource::DescriptorSetRhi::Type kObjectType = Resource::DescriptorSetRhi::Type::PerObject;
        }

        RenderScene::RenderScene(const Rhi::Context& context,
            const Rhi::SwapChain& swapChain,
            Resource::ResourceManager& resourceMgr)
            : m_context(context),
            m_swapChain(swapChain),
            m_resourceMgr(resourceMgr),
            m_materialCache("material state")
        {
            m_shadow = CreateShadowTextures(kShadowMapSize);
            m_target = CreateTargetTextures(m_swapChain.Extent());

            m_frame = CreateFrameState(Resource::kInvalidId);
            m_postProcess = CreatePostProcessState(m_target.m_resolve);
            m_lit = CreateLitState(m_shadow.m_shadowMap);
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;
            auto& sceneProxy = SceneProxy::Get();

            sceneProxy.BuildSceneProxy(m_swapChain.Aspect());

            UpdateFrameState(frameIndex, sceneProxy);
            UpdatePostProcessState(frameIndex, sceneProxy);
            UpdateRenderObjects(frameIndex, sceneProxy);

            sceneProxy.Reset();
        }

        void RenderScene::UpdateFrameState(uint32_t frameIndex, const SceneProxy& proxy)
        {
            auto environmentRecord = proxy.GetEnvironmentRecord();
            if (environmentRecord.has_value())
            {
                m_frame = CreateFrameState(environmentRecord->m_equirectId);
            }

            m_frame.WriteData(frameIndex, proxy.GetFrameData());
        }

        void RenderScene::UpdatePostProcessState(uint32_t frameIndex, const SceneProxy& proxy)
        {
            m_postProcess.WriteData(frameIndex, proxy.GetPostProcessData());
        }

        void RenderScene::Recreate()
        {
            // Build new target and its dependent state first, then commit:
            // the old post-process descriptor set is released before the old
            // target textures, so no live set references a released image view.
            TargetTextures newTarget = CreateTargetTextures(m_swapChain.Extent());
            PostProcessState newPostProcess = CreatePostProcessState(newTarget.m_resolve);

            m_postProcess = std::move(newPostProcess);
            m_target = std::move(newTarget);
        }

        VkDescriptorSetLayout RenderScene::GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type type) const
        {
            return m_resourceMgr.GetDescriptorSetLayout(type);
        }

        ShadowTextures RenderScene::CreateShadowTextures(uint32_t size) const
        {
            Resource::ImageDesc imageDesc{};
            imageDesc.m_extent = { size, size, 1 };
            imageDesc.m_format = m_context.ShadowMapFormat();
            imageDesc.m_aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            imageDesc.m_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageDesc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            Resource::ImageViewDesc imageViewDesc{};
            imageViewDesc.m_type = VK_IMAGE_VIEW_TYPE_2D;
            imageViewDesc.m_fullRange = true;

            Resource::SamplerDesc samplerDesc{};
            samplerDesc.m_magFilter = VK_FILTER_LINEAR;
            samplerDesc.m_minFilter = VK_FILTER_LINEAR;
            samplerDesc.m_mipMode = Resource::SamplerDesc::MipMode::None;
            samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_compare = true;
            samplerDesc.m_compareOp = VK_COMPARE_OP_LESS;

            ShadowTextures shadow{};
            shadow.m_shadowMap = m_resourceMgr.CreateTexture(imageDesc, imageViewDesc, samplerDesc);

            return shadow;
        }

        TargetTextures RenderScene::CreateTargetTextures(VkExtent2D extent) const
        {
            // Color (MSAA) + resolve + depth. Color and resolve share the
            // format: vkCmdResolveImage requires identical src/dst formats
            Resource::ImageDesc colorDesc{};
            colorDesc.m_extent = { extent.width, extent.height, 1 };
            colorDesc.m_format = m_context.HdrFormat();
            colorDesc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorDesc.m_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            colorDesc.m_samples = m_context.SampleCount();
            colorDesc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            Resource::ImageDesc resolveDesc = colorDesc;
            resolveDesc.m_samples = VK_SAMPLE_COUNT_1_BIT;
            resolveDesc.m_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

            Resource::ImageDesc depthDesc{};
            depthDesc.m_extent = { extent.width, extent.height, 1 };
            depthDesc.m_format = m_context.DepthFormat();
            depthDesc.m_aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthDesc.m_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depthDesc.m_samples = m_context.SampleCount();      // MSAA depth matches color
            depthDesc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            Resource::ImageViewDesc viewDesc{};
            viewDesc.m_type = VK_IMAGE_VIEW_TYPE_2D;
            viewDesc.m_fullRange = true;

            Resource::SamplerDesc samplerDesc{};
            samplerDesc.m_magFilter = VK_FILTER_LINEAR;
            samplerDesc.m_minFilter = VK_FILTER_LINEAR;
            samplerDesc.m_mipMode = Resource::SamplerDesc::MipMode::None;
            samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            TargetTextures target{};
            target.m_color = m_resourceMgr.CreateTexture(colorDesc, viewDesc, samplerDesc);
            target.m_resolve = m_resourceMgr.CreateTexture(resolveDesc, viewDesc, samplerDesc);
            target.m_depth = m_resourceMgr.CreateTexture(depthDesc, viewDesc, samplerDesc);

            return target;
        }

        FrameState RenderScene::CreateFrameState(Resource::ResourceId equirectId) const
        {
            FrameState frame{};

            Resource::BufferDesc desc{};
            desc.m_size = sizeof(Resource::Gpu::PerFrame);
            desc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            desc.m_mapped = true;

            for (size_t i = 0; i < frame.m_ubos.size(); ++i)
            {
                frame.m_ubos[i] = m_resourceMgr.CreateUbo(desc);
            }

            auto& brdfLut = m_resourceMgr.GetBrdfLut();
            const auto environments = m_resourceMgr.CreateEnvironments(equirectId);
            frame.m_skybox = environments[0];
            frame.m_irradiance = environments[1];
            frame.m_prefilter = environments[2];

            for (size_t i = 0; i < frame.m_sets.size(); ++i)
            {
                frame.m_sets[i] = m_resourceMgr.CreateDescriptorSet(kFrameType);

                Resource::DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    frame.m_ubos[i].GetBuffer(), 0, sizeof(Resource::Gpu::PerFrame))
                    .WriteImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, brdfLut.GetImageView(), brdfLut.GetSampler())
                    .WriteImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, frame.m_skybox.GetImageView(), frame.m_skybox.GetSampler())
                    .WriteImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, frame.m_irradiance.GetImageView(), frame.m_irradiance.GetSampler())
                    .WriteImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, frame.m_prefilter.GetImageView(), frame.m_prefilter.GetSampler())
                    .UpdateSet(frame.m_sets[i]->GetSet());
            }

            return frame;
        }

        PostProcessState RenderScene::CreatePostProcessState(const Resource::TextureResource& target) const
        {
            PostProcessState postProcess{};

            Resource::BufferDesc desc{};
            desc.m_size = sizeof(Resource::Gpu::PostProcess);
            desc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.m_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            desc.m_mapped = true;

            for (size_t i = 0; i < postProcess.m_ubos.size(); ++i)
            {
                postProcess.m_ubos[i] = m_resourceMgr.CreateUbo(desc);
            }

            for (size_t i = 0; i < postProcess.m_sets.size(); ++i)
            {
                postProcess.m_sets[i] = m_resourceMgr.CreateDescriptorSet(kPostProcessType);

                Resource::DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    postProcess.m_ubos[i].GetBuffer(), 0, sizeof(Resource::Gpu::PostProcess))
                    .WriteImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, target.GetImageView(), target.GetSampler())
                    .UpdateSet(postProcess.m_sets[i]->GetSet());
            }

            return postProcess;
        }

        LitState RenderScene::CreateLitState(const Resource::TextureResource& shadowMap) const
        {
            LitState lit{};

            lit.m_set = m_resourceMgr.CreateDescriptorSet(kLitType);

            Resource::DescriptorWriter writer(m_context.Device());
            writer.WriteImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shadowMap.GetImageView(), shadowMap.GetSampler())
                .UpdateSet(lit.GetSet());

            return lit;
        }

        MaterialState::Handle RenderScene::GetOrCreateMaterialState(const MaterialDesc& desc)
        {
            auto materialStateCreator = [this](const MaterialDesc& desc) -> std::optional<MaterialState>
                {
                    // Material sampling policy, spelled out at the only assembly point
                    Resource::ImageViewDesc imageViewDesc{};
                    imageViewDesc.m_type = VK_IMAGE_VIEW_TYPE_2D;
                    imageViewDesc.m_fullRange = true;

                    Resource::SamplerDesc samplerDesc{};
                    samplerDesc.m_magFilter = VK_FILTER_LINEAR;
                    samplerDesc.m_minFilter = VK_FILTER_LINEAR;
                    samplerDesc.m_mipMode = Resource::SamplerDesc::MipMode::Linear;
                    samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                    samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                    samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

                    MaterialState material{};
                    material.m_set = m_resourceMgr.CreateDescriptorSet(kMaterialType);

                    for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
                    {
                        Resource::TextureResource tex = m_resourceMgr.CreateTexture(desc.m_textureIds[i], imageViewDesc, samplerDesc);
                        material.m_textures[i] = tex.IsEmpty()
                            ? m_resourceMgr.CreateFallback(static_cast<Resource::MaterialSlot>(i), imageViewDesc, samplerDesc)
                            : tex;
                    }

                    Resource::DescriptorWriter writer(m_context.Device());
                    for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
                    {
                        writer.WriteImage(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            material.m_textures[i].GetImageView(), material.m_textures[i].GetSampler());
                    }
                    writer.UpdateSet(material.m_set->GetSet());

                    return material;
                };

            return m_materialCache.GetOrCreate(desc, materialStateCreator);
        }

        ObjectState RenderScene::CreateObjectState() const
        {
            ObjectState object{};

            Resource::BufferDesc desc{};
            desc.m_size = sizeof(Resource::Gpu::PerObject);
            desc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.m_properties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            desc.m_mapped = true;

            for (size_t i = 0; i < object.m_ubos.size(); ++i)
            {
                object.m_ubos[i] = m_resourceMgr.CreateUbo(desc);
            }

            for (size_t i = 0; i < object.m_sets.size(); ++i)
            {
                object.m_sets[i] = m_resourceMgr.CreateDescriptorSet(kObjectType);

                Resource::DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    object.m_ubos[i].GetBuffer(), 0, sizeof(Resource::Gpu::PerObject))
                    .UpdateSet(object.m_sets[i]->GetSet());
            }

            return object;
        }

        RenderObject RenderScene::CreateRenderObject(Resource::ResourceId id)
        {
            RenderObject object{ id };

            MaterialDesc matDesc
            {
                Resource::kInvalidId,
                Resource::kInvalidId,
                Resource::kInvalidId,
                Resource::kInvalidId,
                Resource::kInvalidId
            };

            object.m_material = GetOrCreateMaterialState(matDesc);
            object.m_object = CreateObjectState();
            object.m_mesh = m_resourceMgr.GetOrCreateMesh(Resource::kInvalidId);

            return object;
        }

        void RenderScene::UpdateRenderObjects(uint32_t frameIndex, const SceneProxy& proxy)
        {
            auto& deleteObjects = proxy.GetDeletedObjects();
            for (auto& id : deleteObjects)
            {
                m_objects.Remove(id);
            }

            auto& objectRecords = proxy.GetObjectRecords();
            for (auto& record : objectRecords)
            {
                auto& object = m_objects.FindOrAdd(record.GetId(),
                    [this](Resource::ResourceId id) { return CreateRenderObject(id); });
                if (record.m_meshId.has_value())
                {
                    object.m_mesh = m_resourceMgr.GetOrCreateMesh(record.m_meshId.value());
                }
                if (record.m_textureIds.has_value())
                {
                    MaterialDesc desc{};
                    desc.m_textureIds = record.m_textureIds.value();
                    object.m_material = GetOrCreateMaterialState(desc);
                }
            }

            auto& objectDatas = proxy.GetObjectDatas();
            for (auto& objectData : objectDatas)
            {
                auto object = m_objects.Find(objectData.m_id);

                if (object)
                {
                    object->m_object.WriteData(frameIndex, objectData.m_object);
                }
            }
        }
    }
}
