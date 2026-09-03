#pragma once

#include "rhi/frame_info.h"
#include "resource/cache_table.h"
#include "render/scene_state.h"

#include <unordered_map>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
        class SwapChain;
    }
    namespace Resource
    {
        class ResourceManager;
    }

    namespace Render
    {
        class SceneProxy;

        /// Owns the GPU-side scene state; consumes the SceneProxy each frame.
        /// Passes get their dependencies injected via GetXxx
        class RenderScene
        {
        public:
            RenderScene(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                Resource::ResourceManager& resourceMgr);
            ~RenderScene();

            void Update(const Rhi::FrameInfo& frameInfo);
            void Recreate();

            const ShadowTextures& GetShadow() const { return m_shadow; }
            ShadowTextures& GetShadow() { return m_shadow; }
            const TargetTextures& GetTarget() const { return m_target; }
            TargetTextures& GetTarget() { return m_target; }

            const FrameState& GetFrame() const { return m_frame; }
            const PostProcessState& GetPostProcess() const { return m_postProcess; }
            const LitState& GetLit() const { return m_lit; }
            const std::vector<RenderObject>& GetObjects() const { return m_objects; }

            size_t GetMaterialCount() const { return m_materialCache.Size(); }
            size_t GetObjectCount() const { return m_objects.size(); }

            VkDescriptorSetLayout GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type type) const;

        private:
            ShadowTextures CreateShadowTextures(uint32_t size) const;
            TargetTextures CreateTargetTextures(VkExtent2D extent) const;

            FrameState CreateFrameState(Resource::ResourceId equirectId) const;
            PostProcessState CreatePostProcessState(const Resource::TextureResource& target) const;
            LitState CreateLitState(const Resource::TextureResource& shadowMap) const;
            MaterialState::Handle GetOrCreateMaterialState(const MaterialDesc& desc);
            ObjectState CreateObjectState() const;

            void UpdateFrameState(uint32_t frameIndex, const SceneProxy& proxy);
            void UpdatePostProcessState(uint32_t frameIndex, const SceneProxy& proxy);
            void UpdateRenderObjects(uint32_t frameIndex, const SceneProxy& proxy);

            RenderObject CreateRenderObject(Resource::ResourceId id);
            size_t FindOrAddRenderObject(Resource::ResourceId id);
            void RemoveRenderObject(Resource::ResourceId id);
            size_t FindRenderObject(Resource::ResourceId id);

        private:
            const Rhi::Context& m_context;
            const Rhi::SwapChain& m_swapChain;
            Resource::ResourceManager& m_resourceMgr;

            ShadowTextures m_shadow{};
            TargetTextures m_target{};

            Resource::CacheTable<MaterialState, MaterialDesc, MaterialDesc::Hash> m_materialCache;

            FrameState m_frame{};
            PostProcessState m_postProcess{};
            LitState m_lit{};
            std::vector<RenderObject> m_objects;
            std::unordered_map<Resource::ResourceId, size_t> m_objectIndex;
        };
    }
}
