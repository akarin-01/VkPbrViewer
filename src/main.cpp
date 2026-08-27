#include "core/log.h"
// #include "application/app.h"
#include "core/window.h"
#include "rhi/context.h"
#include "resource/asset_manager.h"
#include "resource/resource_manager.h"

#include <cassert>
#include <cstdint>

namespace
{
    void TestResourceManager()
    {
        using namespace Kita::Pbrv;

        Core::Window window(800, 600, "Vk Pbr Viewer");
        Rhi::Context context(window);
        Resource::AssetManager assets;
        Resource::ResourceManager resources(context, assets);

        const Resource::Mesh::Handle meshAsset =
            assets.LoadMesh("assets/models/DamagedHelmet.gltf");
        assert(meshAsset.IsValid());
        const Resource::ResourceId meshId = meshAsset.GetId();

        // 1. Cache miss: build a new GPU block from the asset.
        Resource::Handle<Resource::MeshData> mesh1 = resources.GetOrCreateMeshData(meshId);
        assert(mesh1.IsValid());
        assert(mesh1->m_indexCount > 0);
        assert(mesh1->m_vertexBuffer.m_buffer != VK_NULL_HANDLE);
        assert(mesh1->m_indexBuffer.m_buffer != VK_NULL_HANDLE);

        // 2. Cache hit: same block shared across handles.
        Resource::Handle<Resource::MeshData> mesh2 = resources.GetOrCreateMeshData(meshId);
        assert(mesh1.GetId() == mesh2.GetId());
        assert(mesh1.Get() == mesh2.Get());

        // 3. Refcount: releasing one handle keeps the block alive via the other.
        mesh1.Reset();
        assert(mesh2.IsValid());
        assert(mesh2->m_indexCount > 0);

        // 4. Last handle released: entry leaves the table (deferred destroy takes it).
        mesh2.Reset();

        // 5. Stale cache entry is detected and rebuilt on demand.
        Resource::Handle<Resource::MeshData> mesh3 = resources.GetOrCreateMeshData(meshId);
        assert(mesh3.IsValid());
        assert(mesh3->m_indexCount > 0);

        // 6. Deferred destroy: flush every frame slot, must not crash or leak.
        for (uint32_t i = 0; i < Rhi::kMaxFramesInFlight; ++i)
        {
            resources.FlushDeferred(i);
        }

        Core::Log::Info("[Test] ResourceManager passed");
    }
}

int main()
{
    try
    {
        // Kita::Pbrv::Application::App app{};
        // app.Run();

        TestResourceManager();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        Kita::Pbrv::Core::Log::Error(e.what());
        return EXIT_FAILURE;
    }
}