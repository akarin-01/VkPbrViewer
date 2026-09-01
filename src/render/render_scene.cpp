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

            m_global.m_frameSet = m_resourceMgr.CreatePerFrameSet(Resource::kInvalidId);

            m_object = CreateObject();
        }

        RenderScene::~RenderScene() = default;

        void RenderScene::Update(const Rhi::FrameInfo& frameInfo)
        {
            auto& frameIndex = frameInfo.m_frameIndex;
            auto& sceneProxy = SceneProxy::Get();

            sceneProxy.BuildSceneProxy(m_swapChain.Aspect());

            UpdateFrameSet(frameIndex, sceneProxy);
            UpdatePostProcessSet(frameIndex, sceneProxy);
            UpdateObject(frameIndex, sceneProxy);

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

        VkDescriptorSetLayout RenderScene::GetEmptyLayout() const
        {
            return m_resourceMgr.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::Empty);
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

        void RenderScene::UpdateObject(uint32_t frameIndex, const SceneProxy& proxy)
        {
            auto& mesh = proxy.GetMeshRecord();
            if (mesh.has_value())
            {
                m_object.m_mesh = m_resourceMgr.GetOrCreateMesh(mesh->m_meshId);
            }

            auto& mat = proxy.GetMaterialRecord();
            if (mat.has_value())
            {
                Resource::MaterialDesc desc{};
                desc.m_textureIds = mat->m_textureIds;
                m_object.m_materialSet = m_resourceMgr.GetOrCreatePerMaterialSet(desc);
            }

            m_object.m_objectSet.WriteData(frameIndex, proxy.GetObjectData());
        }
    }
}
