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

        constexpr Resource::SamplerDesc kDefaultSampler{};   // linear / repeat / mip-Linear

        {
            // 1. Asset path: image + view + sampler assembled from the texture id.
            const Resource::TextureAsset::Handle albedo =
                assets.LoadTexture("assets/models/Default_albedo.jpg", Resource::TextureAsset::Type::Srgb);
            assert(albedo.IsValid());

            const Resource::TextureResource tex1 =
                resources.CreateTexture(albedo.GetId(), {}, kDefaultSampler);
            assert(!tex1.IsEmpty());
            assert(tex1.GetImage() != VK_NULL_HANDLE);
            assert(tex1.GetImageView() != VK_NULL_HANDLE);
            assert(tex1.GetSampler() != VK_NULL_HANDLE);
            assert(tex1.m_image->m_format == VK_FORMAT_R8G8B8A8_SRGB);
            assert(tex1.m_image->m_extent.width == 2048 && tex1.m_image->m_extent.height == 2048);
            assert(tex1.m_image->m_mipLevels == 12);    // 2048 = 2^11 -> 12 mips

            // 2. Same asset id: image and sampler shared, view always fresh.
            const Resource::TextureResource tex2 =
                resources.CreateTexture(albedo.GetId(), {}, kDefaultSampler);
            assert(tex2.GetImage() == tex1.GetImage());
            assert(tex2.GetSampler() == tex1.GetSampler());
            assert(tex2.GetImageView() != tex1.GetImageView());

            // 3. A different asset maps to its own image with its own format.
            const Resource::TextureAsset::Handle normalTex =
                assets.LoadTexture("assets/models/Default_normal.jpg", Resource::TextureAsset::Type::Normal);
            assert(normalTex.IsValid());
            const Resource::TextureResource tex3 =
                resources.CreateTexture(normalTex.GetId(), {}, kDefaultSampler);
            assert(tex3.GetImage() != tex1.GetImage());
            assert(tex3.m_image->m_format == VK_FORMAT_R8G8B8A8_UNORM);

            // 4. Invalid id: empty texture (GetOrCreate* convention).
            const Resource::TextureResource texEmpty =
                resources.CreateTexture(Resource::kInvalidId, {}, kDefaultSampler);
            assert(texEmpty.IsEmpty());

            // 5. Data path: fresh image per call, sampler deduped by desc.
            constexpr uint8_t white[] = { 255, 255, 255, 255 };
            Resource::ImageDesc imageDesc{};
            imageDesc.m_extent = { 1, 1, 1 };
            imageDesc.m_format = VK_FORMAT_R8G8B8A8_UNORM;
            imageDesc.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageDesc.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;

            const Resource::TextureResource proc1 =
                resources.CreateTexture(imageDesc, {}, kDefaultSampler, white, sizeof(white));
            const Resource::TextureResource proc2 =
                resources.CreateTexture(imageDesc, {}, kDefaultSampler, white, sizeof(white));
            assert(!proc1.IsEmpty());
            assert(proc1.GetImageView() != VK_NULL_HANDLE);
            assert(proc2.GetImage() != proc1.GetImage());         // not deduped: fresh image each call
            assert(proc2.GetSampler() == proc1.GetSampler());     // deduped by desc
            assert(proc1.GetSampler() == tex1.GetSampler());      // same desc over both paths: one shared sampler
        }   // all textures released here

        // 6. Age out the graveyard.
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
