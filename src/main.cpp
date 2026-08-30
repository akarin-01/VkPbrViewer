#include "core/log.h"
#include "application/app.h"
#include "core/window.h"
#include "rhi/context.h"
#include "resource/asset_manager.h"
#include "resource/descriptor_manager.h"
#include "resource/resource_manager.h"

#include <cassert>
#include <cstdint>

namespace
{
    void TestTexture()
    {
        using namespace Kita::Pbrv;

        Core::Window window(800, 600, "Vk Pbr Viewer");
        Rhi::Context context(window);
        Resource::AssetManager assets;
        Resource::DescriptorManager descriptorMgr(context);
        Resource::ResourceManager resources(context, assets, descriptorMgr);

        // 1. GetOrCreateImage from a texture asset: format/extent/mips derived.
        const Resource::TextureAsset::Handle albedo =
            assets.LoadTexture("assets/models/Default_albedo.jpg", Resource::TextureAsset::Type::Srgb);
        assert(albedo.IsValid());

        Resource::ImageResource::Handle img1 = resources.GetOrCreateImage(albedo.GetId());
        assert(img1.IsValid());
        assert(img1->m_image != VK_NULL_HANDLE);
        assert(img1->m_format == VK_FORMAT_R8G8B8A8_SRGB);
        assert(img1->m_extent.width == 2048 && img1->m_extent.height == 2048);
        assert(img1->m_mipLevels == 12);    // 2048 = 2^11 -> 12 mips

        // 2. Dedup: the same asset id resolves to the same entry.
        Resource::ImageResource::Handle img2 = resources.GetOrCreateImage(albedo.GetId());
        assert(img1.GetId() == img2.GetId());
        img1.Reset();
        assert(img2.IsValid());
        assert(img2->m_image != VK_NULL_HANDLE);

        // 3. A different asset maps to its own entry with its own format.
        const Resource::TextureAsset::Handle normalTex =
            assets.LoadTexture("assets/models/Default_normal.jpg", Resource::TextureAsset::Type::Normal);
        assert(normalTex.IsValid());
        Resource::ImageResource::Handle img3 = resources.GetOrCreateImage(normalTex.GetId());
        assert(img3.GetId() != img2.GetId());
        assert(img3->m_format == VK_FORMAT_R8G8B8A8_UNORM);

        // 4. Release everything, then age out the graveyard.
        img2.Reset();
        img3.Reset();
        for (uint32_t i = 0; i < Rhi::kMaxFramesInFlight; ++i)
        {
            resources.FlushGraveyard();
        }

        Core::Log::Info("[Test] Texture passed");
    }
}

int main()
{
    try
    {
        TestTexture();

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
