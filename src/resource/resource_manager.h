#pragma once

#include "resource/handle_table.h"
#include "resource/resource_types.h"
#include "resource/resource_id.h"
#include "resource/resource_graveyard.h"

#include <unordered_map>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        class AssetManager;
        struct MeshAsset;

        class ResourceManager
        {
        public:
            ResourceManager(const Rhi::Context& context,
                const AssetManager& assetMgr);
            ~ResourceManager();

            BufferResource::Handle CreateBuffer(const BufferDesc& desc,
                const void* data = nullptr, size_t size = 0);

            MeshResource::Handle GetOrCreateMesh(ResourceId meshId);

            void FlushGraveyard();

        private:
            MeshResource CreateMeshResource(const MeshAsset& asset);

        private:
            const Rhi::Context& m_context;
            const AssetManager& m_assetMgr;

            ResourceGraveyard m_graveyard;      // Graveyard must be destroyed after table

            HandleTable<BufferResource> m_bufferTable;

            // ------------- Cache --------------------
            HandleTable<MeshResource> m_meshTable;
            std::unordered_map<ResourceId, ResourceId> m_meshIds;
        };
    }
}
