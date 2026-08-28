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

        class ResourceManager
        {
        public:
            ResourceManager(const Rhi::Context& context,
                AssetManager& assetMgr);
            ~ResourceManager();

            MeshResource::Handle GetOrCreateMeshResource(ResourceId meshId);

            void FlushGraveyard();

        private:
            const Rhi::Context& m_context;
            AssetManager& m_assetMgr;

            ResourceGraveyard m_graveyard;      // Graveyard must be destroyed after table

            HandleTable<MeshResource> m_meshTable;
            std::unordered_map<ResourceId, ResourceId> m_meshIds;
        };
    }
}
