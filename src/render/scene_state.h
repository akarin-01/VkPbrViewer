#pragma once

#include "resource/constants.h"
#include "resource/gpu_layouts.h"
#include "resource/handle.h"
#include "resource/resource_id.h"
#include "resource/resource_types.h"

#include <array>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Render
    {
        // ------------------------- Textures -----------------------------

        struct TargetTextures
        {
            Resource::TextureResource m_color{};
            Resource::TextureResource m_resolve{};
            Resource::TextureResource m_depth{};

            VkImage GetColorImage() const { return m_color.GetImage(); }
            VkImageView GetColorImageView() const { return m_color.GetImageView(); }
            VkSampler GetColorSampler() const { return m_color.GetSampler(); }
            VkFormat GetColorFormat() const { return m_color.GetFormat(); }

            VkImage GetResolveImage() const { return m_resolve.GetImage(); }
            VkImageView GetResolveImageView() const { return m_resolve.GetImageView(); }
            VkSampler GetResolveSampler() const { return m_resolve.GetSampler(); }

            VkImage GetDepthImage() const { return m_depth.GetImage(); }
            VkImageView GetDepthImageView() const { return m_depth.GetImageView(); }
            VkSampler GetDepthSampler() const { return m_depth.GetSampler(); }
            VkFormat GetDepthFormat() const { return m_depth.GetFormat(); }
        };

        struct ShadowTextures
        {
            Resource::TextureResource m_shadowMap{};

            VkImage GetImage() const { return m_shadowMap.GetImage(); }
            VkImageView GetImageView() const { return m_shadowMap.GetImageView(); }
            VkSampler GetSampler() const { return m_shadowMap.GetSampler(); }
            VkFormat GetFormat() const { return m_shadowMap.GetFormat(); }
            VkExtent3D GetExtent() const { return m_shadowMap.GetExtent(); }
        };

        // ------------------------- State -----------------------------

        // Set 0: K UBO slots + K descriptor sets; IBL textures fixed at creation
        struct FrameState
        {
            std::array<Resource::UboResource, Rhi::kMaxFramesInFlight> m_ubos{};
            Resource::TextureResource m_skybox{};
            Resource::TextureResource m_irradiance{};
            Resource::TextureResource m_prefilter{};
            std::array<Resource::DescriptorSetRhi::Handle, Rhi::kMaxFramesInFlight> m_sets{};

            void WriteData(uint32_t frameIndex, const Resource::Gpu::PerFrame& data)
            {
                m_ubos[frameIndex].Write(&data, sizeof(data));
            }

            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]->GetSet(); }
        };

        // Set 1: K UBO slots + K descriptor sets, bound once at creation
        struct PostProcessState
        {
            std::array<Resource::UboResource, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<Resource::DescriptorSetRhi::Handle, Rhi::kMaxFramesInFlight> m_sets{};

            void WriteData(uint32_t frameIndex, const Resource::Gpu::PostProcess& data)
            {
                m_ubos[frameIndex].Write(&data, sizeof(data));
            }

            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]->GetSet(); }
        };

        struct LitState
        {
            Resource::DescriptorSetRhi::Handle m_set{};

            const VkDescriptorSet& GetSet() const { return m_set->GetSet(); }
        };

        struct MaterialDesc
        {
            // One texture id per material slot; kInvalidId = fallback
            std::array<Resource::ResourceId, Resource::kMaterialSlotCount> m_textureIds{};

            bool operator==(const MaterialDesc& other) const
            {
                return m_textureIds == other.m_textureIds;
            }

            struct Hash
            {
                size_t operator()(const MaterialDesc& d) const
                {
                    size_t h = 1469598103934665603ull;
                    auto mix = [&h](uint64_t v) { h ^= v; h *= 1099511628211ull; };
                    for (Resource::ResourceId id : d.m_textureIds)
                    {
                        mix(id);
                    }
                    return h;
                }
            };
        };

        // Set 2 (one immutable set; empty slots share the manager's fallbacks)
        struct MaterialState
        {
            using Handle = Resource::Handle<MaterialState>;

            std::array<Resource::TextureResource, Resource::kMaterialSlotCount> m_textures{};
            Resource::DescriptorSetRhi::Handle m_set{};

            const VkDescriptorSet& GetSet() const { return m_set->GetSet(); }
        };

        // Set 3: K UBO slots + K descriptor sets, bound once at creation
        struct ObjectState
        {
            std::array<Resource::UboResource, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<Resource::DescriptorSetRhi::Handle, Rhi::kMaxFramesInFlight> m_sets{};

            void WriteData(uint32_t frameIndex, const Resource::Gpu::PerObject& data)
            {
                m_ubos[frameIndex].Write(&data, sizeof(data));
            }

            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]->GetSet(); }
        };

        /// One render-scene entity: id + mesh + shared material state handle
        /// + per-object set state; exposes semantic getters for pass use.
        struct RenderObject
        {
            Resource::ResourceId m_id{ Resource::kInvalidId };

            MaterialState::Handle m_material{};
            ObjectState m_object{};

            Resource::MeshResource::Handle m_mesh{};

            const VkDescriptorSet& GetMaterialSet() const { return m_material->GetSet(); }
            Resource::ResourceId GetMaterialId() const { return m_material.GetId(); }

            const VkDescriptorSet& GetObjectSet(uint32_t frameIndex) const { return m_object.GetSet(frameIndex); }

            bool HasMesh() const { return m_mesh.IsValid(); }
            Resource::ResourceId GetMeshId() const { return m_mesh.GetId(); }
            VkBuffer GetVertexBuffer() const { return m_mesh->GetVertexBuffer(); }
            VkBuffer GetIndexBuffer() const { return m_mesh->GetIndexBuffer(); }
            uint32_t GetIndexCount() const { return m_mesh->GetIndexCount(); }
        };
    }
}
