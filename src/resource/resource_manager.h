#pragma once

#include "resource/mapping_deferred_table.h"
#include "resource/blocks.h"
#include "resource/resource_id.h"
#include "resource/handle.h"

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        class AssetManager;
        class DescriptorManager;

        class ResourceManager
        {
        public:
            ResourceManager(const Rhi::Context& context,
                AssetManager& assetMgr);
            ~ResourceManager();

            Handle<MeshData> GetOrCreateMeshData(ResourceId meshId);

            void FlushDeferred(uint32_t frameIndex);

        private:
            const Rhi::Context& m_context;
            AssetManager& m_assetMgr;

            MappingDeferredTable<ResourceId, MeshData> m_meshTable;
        };
    }
}
