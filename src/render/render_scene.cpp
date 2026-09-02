#include "render_scene.h"

#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/resource_manager.h"
#include "render/scene_proxy.h"

namespace Kita::Pbrv
{
    namespace Render
    {
        RenderScene::RenderScene(const Rhi::Context& context,
            const Rhi::SwapChain& swapChain,
            Resource::ResourceManager& resourceMgr)
            : m_context(context),
            m_swapChain(swapChain),
            m_resourceMgr(resourceMgr)
        {
            Recreate();

            constexpr uint32_t kShadowMapSize = 2048;

            Resource::ImageDesc imageDesc{};
            imageDesc.m_extent = { kShadowMapSize, kShadowMapSize, 1 };
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

            m_global.m_shadowMap = m_resourceMgr.CreateTexture(imageDesc, imageViewDesc, samplerDesc);
            m_global.m_frameSet = m_resourceMgr.CreatePerFrameSet(Resource::kInvalidId);
            m_global.m_litSet = m_resourceMgr.CreateLitSet(m_global.m_shadowMap);
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;
            auto& sceneProxy = SceneProxy::Get();

            sceneProxy.BuildSceneProxy(m_swapChain.Aspect());

            UpdateFrameSet(frameIndex, sceneProxy);
            UpdatePostProcessSet(frameIndex, sceneProxy);
            UpdateObjects(frameIndex, sceneProxy);

            sceneProxy.Reset();
        }

        void RenderScene::UpdateFrameSet(uint32_t frameIndex, const SceneProxy& proxy)
        {
            auto environmentRecord = proxy.GetEnvironmentRecord();
            if (environmentRecord.has_value())
            {
                m_global.m_frameSet = m_resourceMgr.CreatePerFrameSet(environmentRecord->m_equirectId);
            }

            m_global.m_frameSet.WriteData(frameIndex, proxy.GetFrameData());
        }

        void RenderScene::UpdatePostProcessSet(uint32_t frameIndex, const SceneProxy& proxy)
        {
            m_global.m_postProcessSet.WriteData(frameIndex, proxy.GetPostProcessData());
        }

        void RenderScene::Recreate()
        {
            Resource::TargetDesc desc{};
            desc.m_extent = m_swapChain.Extent();
            desc.m_colorFormat = m_context.HdrFormat();
            desc.m_depthFormat = m_context.DepthFormat();
            desc.m_msaaSamples = m_context.SampleCount();

            // Linear + clamp + no mips: render target sampling
            Resource::SamplerDesc samplerDesc{};
            samplerDesc.m_magFilter = VK_FILTER_LINEAR;
            samplerDesc.m_minFilter = VK_FILTER_LINEAR;
            samplerDesc.m_mipMode = Resource::SamplerDesc::MipMode::None;
            samplerDesc.m_addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerDesc.m_addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            desc.m_samplerDesc = samplerDesc;

            m_global.m_target = m_resourceMgr.CreateTarget(desc);

            m_global.m_postProcessSet = m_resourceMgr.CreatePostProcessSet(m_global.m_target.m_resolveTexture);
        }

        VkDescriptorSetLayout RenderScene::GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type type) const
        {
            return m_resourceMgr.GetDescriptorSetLayout(type);
        }

        ObjectState RenderScene::CreateObject() const
        {
            ObjectState object{};

            Resource::MaterialDesc matDesc
            {
                Resource::kInvalidId,
                Resource::kInvalidId,
                Resource::kInvalidId,
                Resource::kInvalidId,
                Resource::kInvalidId
            };
            object.m_materialSet = m_resourceMgr.GetOrCreatePerMaterialSet(matDesc);
            object.m_objectSet = m_resourceMgr.CreatePerObjectSet();
            object.m_mesh = m_resourceMgr.GetOrCreateMesh(Resource::kInvalidId);

            return object;
        }

        void RenderScene::UpdateObjects(uint32_t frameIndex, const SceneProxy& proxy)
        {
            auto& deleteObjects = proxy.GetDeletedObjects();
            for (auto& id : deleteObjects)
            {
                m_objects.erase(id);
            }

            auto& meshRecords = proxy.GetMeshRecords();
            for (auto& meshRecord : meshRecords)
            {
                auto it = m_objects.find(meshRecord.m_id);
                if (it == m_objects.end())
                {
                    it = m_objects.emplace(meshRecord.m_id, CreateObject()).first;
                }

                auto& object = it->second;
                object.m_mesh = m_resourceMgr.GetOrCreateMesh(meshRecord.m_meshId);
            }

            auto& materialRecords = proxy.GetMaterialRecords();
            for (auto& materialRecord : materialRecords)
            {
                auto it = m_objects.find(materialRecord.m_id);
                if (it == m_objects.end())
                {
                    it = m_objects.emplace(materialRecord.m_id, CreateObject()).first;
                }

                Resource::MaterialDesc desc{};
                desc.m_textureIds = materialRecord.m_textureIds;
                auto& object = it->second;
                object.m_materialSet = m_resourceMgr.GetOrCreatePerMaterialSet(desc);
            }

            auto& objectDatas = proxy.GetObjectDatas();
            for (auto& objectData : objectDatas)
            {
                auto it = m_objects.find(objectData.m_id);
                if (it == m_objects.end())
                {
                    continue;   // records create new states above; skip anything else
                }

                auto& object = it->second;
                object.m_objectSet.WriteData(frameIndex, objectData.m_object);
            }
        }
    }
}
