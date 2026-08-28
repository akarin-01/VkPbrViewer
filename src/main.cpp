#include "core/log.h"
#include "application/app.h"
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

        const Resource::MeshAsset::Handle meshAsset =
            assets.LoadMesh("assets/models/DamagedHelmet.gltf");
        assert(meshAsset.IsValid());
        const Resource::ResourceId meshId = meshAsset.GetId();

        // 1. Cache miss: build a new GPU block from the asset.
        Resource::MeshResource::Handle mesh1 = resources.GetOrCreateMeshResource(meshId);
        assert(mesh1.IsValid());
        assert(mesh1->m_indexCount > 0);
        assert(mesh1->m_vertexBuffer.m_buffer != VK_NULL_HANDLE);
        assert(mesh1->m_indexBuffer.m_buffer != VK_NULL_HANDLE);

        // 2. Cache hit: same block shared across handles.
        Resource::MeshResource::Handle mesh2 = resources.GetOrCreateMeshResource(meshId);
        assert(mesh1.GetId() == mesh2.GetId());
        assert(mesh1.Get() == mesh2.Get());

        // 3. Refcount: releasing one handle keeps the block alive via the other.
        mesh1.Reset();
        assert(mesh2.IsValid());
        assert(mesh2->m_indexCount > 0);

        // 4. Last handle released: entry leaves the table (deferred destroy takes it).
        mesh2.Reset();

        // 5. Stale cache entry is detected and rebuilt on demand.
        Resource::MeshResource::Handle mesh3 = resources.GetOrCreateMeshResource(meshId);
        assert(mesh3.IsValid());
        assert(mesh3->m_indexCount > 0);

        // 6. Deferred destroy: age the graveyard out, must not crash or leak.
        for (uint32_t i = 0; i < Rhi::kMaxFramesInFlight; ++i)
        {
            resources.FlushGraveyard();
        }

        Core::Log::Info("[Test] ResourceManager passed");
    }

    void TestAssetManager()
    {
        using namespace Kita::Pbrv;

        Resource::AssetManager assets;

        // 1. Cache hit: the same path shares one asset entry.
        Resource::MeshAsset::Handle mesh1 = assets.LoadMesh("assets/models/DamagedHelmet.gltf");
        Resource::MeshAsset::Handle mesh2 = assets.LoadMesh("assets/models/DamagedHelmet.gltf");
        assert(mesh1.IsValid() && mesh2.IsValid());
        assert(mesh1.GetId() == mesh2.GetId());

        // 2. Refcount: releasing one handle keeps the asset alive.
        mesh1.Reset();
        assert(mesh2.IsValid() && mesh2->GetVertexCount() > 0);

        // 3. Stale entry: releasing the last handle removes the entry; the
        //    next load rebuilds it under a fresh id.
        const Resource::ResourceId firstId = mesh2.GetId();
        mesh2.Reset();
        Resource::MeshAsset::Handle mesh3 = assets.LoadMesh("assets/models/DamagedHelmet.gltf");
        assert(mesh3.IsValid());
        assert(mesh3.GetId() != firstId);

        // 4. Load failure throws; the failed path is not cached.
        bool threw = false;
        try
        {
            assets.LoadMesh("assets/models/NoExist.gltf");
        }
        catch (const std::exception&)
        {
            threw = true;
        }
        assert(threw);

        // 5. Texture keys: the same key shares an entry, a different type
        //    creates its own entry.
        Resource::TextureAsset::Handle tex1 =
            assets.LoadTexture("assets/models/Default_albedo.jpg", Resource::TextureAsset::Type::Srgb);
        Resource::TextureAsset::Handle tex2 =
            assets.LoadTexture("assets/models/Default_albedo.jpg", Resource::TextureAsset::Type::Srgb);
        Resource::TextureAsset::Handle tex3 =
            assets.LoadTexture("assets/models/Default_albedo.jpg", Resource::TextureAsset::Type::Linear);
        assert(tex1.IsValid() && tex2.IsValid() && tex3.IsValid());
        assert(tex1.GetId() == tex2.GetId());
        assert(tex1.GetId() != tex3.GetId());

        Core::Log::Info("[Test] AssetManager passed");
    }
}

int main()
{
    try
    {
        // TestAssetManager();
        // TestResourceManager();

        Kita::Pbrv::Application::App app{};
        app.Run();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        Kita::Pbrv::Core::Log::Error(e.what());
        return EXIT_FAILURE;
    }
}
