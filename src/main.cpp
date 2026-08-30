#include "core/log.h"
#include "application/app.h"
#include "core/window.h"
#include "rhi/context.h"
#include "resource/asset_manager.h"
#include "resource/descriptor_manager.h"
#include "resource/resource_manager.h"

#include <cassert>
#include <cstdint>

#include <cstring>
#include <vector>

namespace
{
    void TestResourceManager()
    {
        using namespace Kita::Pbrv;

        Core::Window window(800, 600, "Vk Pbr Viewer");
        Rhi::Context context(window);
        Resource::AssetManager assets;
        Resource::DescriptorManager descriptor(context);
        Resource::ResourceManager resources(context, assets, descriptor);

        // 1. CreateBuffer: host-visible mapped UBO, written through the mapping.
        Resource::BufferDesc uboDesc{};
        uboDesc.m_size = 64;
        uboDesc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uboDesc.m_properties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        uboDesc.m_mapped = true;
        Resource::BufferResource::Handle ubo = resources.CreateBuffer(uboDesc);
        assert(ubo.IsValid());
        assert(ubo->m_buffer != VK_NULL_HANDLE);
        assert(ubo->m_mapped != nullptr);

        const uint32_t pattern = 0xCAFEBABE;
        std::memcpy(ubo->m_mapped, &pattern, sizeof(pattern));
        assert(std::memcmp(ubo->m_mapped, &pattern, sizeof(pattern)) == 0);

        // 2. CreateBuffer with initial data: device-local upload through staging.
        const std::vector<float> vbData = { 0.0f, 1.0f, 2.0f, 3.0f };
        Resource::BufferDesc vbDesc{};
        vbDesc.m_size = vbData.size() * sizeof(float);
        vbDesc.m_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vbDesc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        Resource::BufferResource::Handle vb = resources.CreateBuffer(vbDesc,
            vbData.data(), vbData.size() * sizeof(float));
        assert(vb.IsValid());
        assert(vb->m_buffer != VK_NULL_HANDLE);
        assert(vb->m_mapped == nullptr);    // device-local: not mapped

        // 3. GetOrCreateMesh: cache miss builds a mesh whose buffers are handles.
        const Resource::MeshAsset::Handle meshAsset =
            assets.LoadMesh("assets/models/DamagedHelmet.gltf");
        assert(meshAsset.IsValid());
        const Resource::ResourceId meshId = meshAsset.GetId();

        Resource::MeshResource::Handle mesh1 = resources.GetOrCreateMesh(meshId);
        assert(mesh1.IsValid());
        assert(mesh1->m_indexCount > 0);
        assert(mesh1->m_vertexBuffer.IsValid());
        assert(mesh1->m_indexBuffer.IsValid());
        assert(mesh1->GetVertexBuffer() != VK_NULL_HANDLE);
        assert(mesh1->GetIndexBuffer() != VK_NULL_HANDLE);

        // 4. Cache hit: one entry shared across handles; refcount keeps it alive.
        Resource::MeshResource::Handle mesh2 = resources.GetOrCreateMesh(meshId);
        assert(mesh1.GetId() == mesh2.GetId());
        mesh1.Reset();
        assert(mesh2.IsValid());
        assert(mesh2->m_indexCount > 0);

        // 5. Last handle released: entry leaves the table, stale id rebuilds it.
        mesh2.Reset();
        Resource::MeshResource::Handle mesh3 = resources.GetOrCreateMesh(meshId);
        assert(mesh3.IsValid());
        assert(mesh3->m_indexCount > 0);
        assert(mesh3->m_vertexBuffer.IsValid());

        // 6. Deferred destroy: release everything, then age out the graveyard.
        mesh3.Reset();      // mesh entry death cascades into its buffer handles -> graveyard
        vb.Reset();
        ubo.Reset();
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

    void TestImageManager()
    {
        using namespace Kita::Pbrv;

        Core::Window window(800, 600, "Vk Pbr Viewer");
        Rhi::Context context(window);
        Resource::AssetManager assets;
        Resource::DescriptorManager descriptorMgr(context);
        Resource::ResourceManager resources(context, assets, descriptorMgr);

        // 1. CreateImage with data: upload + mip chain (4x4 RGBA8, 3 mips).
        Resource::ImageDesc desc{};
        desc.m_extent = { 4, 4, 1 };
        desc.m_format = VK_FORMAT_R8G8B8A8_UNORM;
        desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.m_mipLevels = 3;
        desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;

        const std::vector<uint8_t> pixels(4 * 4 * 4, 128);
        Resource::ImageResource::Handle img = resources.CreateImage(desc, pixels.data(), pixels.size());
        assert(img.IsValid());
        assert(img->m_image != VK_NULL_HANDLE);
        assert(img->m_extent.width == 4 && img->m_extent.height == 4);
        assert(img->m_mipLevels == 3);

        // 2. Refcount: a copied handle keeps the entry alive.
        Resource::ImageResource::Handle img2 = img;
        assert(img.GetId() == img2.GetId());
        img.Reset();
        assert(img2.IsValid());
        assert(img2->m_image != VK_NULL_HANDLE);

        // 3. Empty image (no data): allocation only, layout left undefined.
        Resource::ImageDesc emptyDesc{};
        emptyDesc.m_extent = { 8, 8, 1 };
        emptyDesc.m_format = VK_FORMAT_R8G8B8A8_UNORM;
        emptyDesc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        emptyDesc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        Resource::ImageResource::Handle empty = resources.CreateImage(emptyDesc);
        assert(empty.IsValid());
        assert(empty->m_image != VK_NULL_HANDLE);
        assert(empty->m_mipLevels == 1);

        // 4. Release everything, then age out the graveyard (K frames).
        img2.Reset();
        empty.Reset();
        for (uint32_t i = 0; i < Rhi::kMaxFramesInFlight; ++i)
        {
            resources.FlushGraveyard();
        }

        Core::Log::Info("[Test] ImageManager passed");
    }

    void TestImageView()
    {
        using namespace Kita::Pbrv;

        Core::Window window(800, 600, "Vk Pbr Viewer");
        Rhi::Context context(window);
        Resource::AssetManager assets;
        Resource::DescriptorManager descriptorMgr(context);
        Resource::ResourceManager resources(context, assets, descriptorMgr);

        // 1. Image with mips, then a full-range view.
        Resource::ImageDesc desc{};
        desc.m_extent = { 8, 8, 1 };
        desc.m_format = VK_FORMAT_R8G8B8A8_UNORM;
        desc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.m_mipLevels = 2;
        desc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;

        const std::vector<uint8_t> pixels(8 * 8 * 4, 64);
        Resource::ImageResource::Handle image = resources.CreateImage(desc, pixels.data(), pixels.size());
        assert(image.IsValid());

        Resource::ImageViewDesc fullDesc{};
        Resource::ImageViewResource::Handle view = resources.CreateImageView(fullDesc, image);
        assert(view.IsValid());
        assert(view->m_imageView != VK_NULL_HANDLE);
        assert(view->m_image.GetId() == image.GetId());   // holds a ref to the image

        // 2. The view keeps the image alive after the caller releases it.
        image.Reset();
        assert(view->m_image.IsValid());
        assert(view->m_image->m_image != VK_NULL_HANDLE);

        // 3. Partial-range view: mip level 1 only.
        Resource::ImageViewDesc partialDesc{};
        partialDesc.m_fullRange = false;
        partialDesc.m_baseMipLevel = 1;
        partialDesc.m_levelCount = 1;
        Resource::ImageViewResource::Handle view2 = resources.CreateImageView(partialDesc, view->m_image);
        assert(view2.IsValid());
        assert(view2->m_imageView != VK_NULL_HANDLE);

        // 4. Release the views but deliberately keep the graveyard full: the
        //    scope exits with pending views (holding image refs) in the
        //    graveyard and the image entry still alive. During teardown the
        //    tables die before the graveyard, so destroying a pending view
        //    releases its image handle against a dead table — unless the
        //    manager drains the graveyard first (FlushAll) or the graveyard
        //    takes over raw handles only.
        view.Reset();
        view2.Reset();
        // NOTE: no FlushGraveyard here — the residue is left for the destructor.

        Core::Log::Info("[Test] ImageView passed");
    }
}

int main()
{
    try
    {
        // TestAssetManager();
        // TestResourceManager();
        TestImageView();

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
