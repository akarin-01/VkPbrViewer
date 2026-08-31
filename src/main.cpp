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

            // 6. Handle path: assemble over an existing image handle.
            const Resource::ImageRhi::Handle rawImage = resources.CreateImage(imageDesc, white, sizeof(white));
            const Resource::TextureResource tex4 = resources.CreateTexture(rawImage, {}, kDefaultSampler);
            assert(!tex4.IsEmpty());
            assert(tex4.GetImage() == rawImage->m_image);
            assert(tex4.GetSampler() == tex1.GetSampler());          // one shared default sampler
            const Resource::TextureResource tex5 = resources.CreateTexture(rawImage, {}, kDefaultSampler);
            assert(tex5.GetImage() == tex4.GetImage());              // same image handle
            assert(tex5.GetImageView() != tex4.GetImageView());      // fresh view per call
        }   // all textures released here

        // 6. Age out the graveyard.
        for (uint32_t i = 0; i < Rhi::kMaxFramesInFlight; ++i)
        {
            resources.FlushGraveyard();
        }

        Core::Log::Info("[Test] Texture passed");
    }

    void TestMaterialSet()
    {
        using namespace Kita::Pbrv;

        Core::Window window(800, 600, "Vk Pbr Viewer");
        Rhi::Context context(window);
        Resource::AssetManager assets;
        Resource::DescriptorManager descriptorMgr(context);
        Resource::ResourceManager resources(context, assets, descriptorMgr);

        {
            // 1. Empty material: every slot resolves to the shared fallback image.
            const Resource::MaterialDesc emptyDesc{};    // ids default to kInvalidId
            const Resource::PerMaterialSet::Handle mat1 =
                resources.GetOrCreatePerMaterialSet(emptyDesc);
            assert(mat1.IsValid());
            assert(mat1->m_layout != VK_NULL_HANDLE);
            assert(mat1->m_set != VK_NULL_HANDLE);
            for (uint32_t i = 0; i < Resource::kMaterialSlotCount; ++i)
            {
                assert(!mat1->m_textures[i].IsEmpty());
                assert(mat1->m_textures[i].GetImageView() != VK_NULL_HANDLE);
                assert(mat1->m_textures[i].GetSampler() != VK_NULL_HANDLE);
            }
            assert(mat1->m_textures[static_cast<size_t>(Resource::MaterialSlot::Albedo)].m_image->m_format == VK_FORMAT_R8G8B8A8_SRGB);
            assert(mat1->m_textures[static_cast<size_t>(Resource::MaterialSlot::Albedo)].m_image->m_extent.width == 1);
            assert(mat1->m_textures[static_cast<size_t>(Resource::MaterialSlot::AO)].m_image->m_format == VK_FORMAT_R8_UNORM);

            // 2. Same desc dedups to the same entry.
            const Resource::PerMaterialSet::Handle mat2 =
                resources.GetOrCreatePerMaterialSet(emptyDesc);
            assert(mat2.GetId() == mat1.GetId());

            // 3. Real textures for albedo/normal: assets wired, other slots fallback.
            const Resource::TextureAsset::Handle albedo =
                assets.LoadTexture("assets/models/Default_albedo.jpg", Resource::TextureAsset::Type::Srgb);
            const Resource::TextureAsset::Handle normalTex =
                assets.LoadTexture("assets/models/Default_normal.jpg", Resource::TextureAsset::Type::Normal);
            assert(albedo.IsValid() && normalTex.IsValid());

            Resource::MaterialDesc fullDesc{};
            fullDesc.m_textureIds[static_cast<size_t>(Resource::MaterialSlot::Albedo)] = albedo.GetId();
            fullDesc.m_textureIds[static_cast<size_t>(Resource::MaterialSlot::Normal)] = normalTex.GetId();
            const Resource::PerMaterialSet::Handle mat3 =
                resources.GetOrCreatePerMaterialSet(fullDesc);
            assert(mat3.GetId() != mat1.GetId());
            assert(mat3->m_textures[static_cast<size_t>(Resource::MaterialSlot::Albedo)].m_image->m_format == VK_FORMAT_R8G8B8A8_SRGB);
            assert(mat3->m_textures[static_cast<size_t>(Resource::MaterialSlot::Albedo)].m_image->m_extent.width == 2048);    // real asset
            assert(mat3->m_textures[static_cast<size_t>(Resource::MaterialSlot::Normal)].m_image->m_format == VK_FORMAT_R8G8B8A8_UNORM);
            assert(mat3->m_textures[static_cast<size_t>(Resource::MaterialSlot::MetallicRoughness)].m_image->m_extent.width == 1);   // fallback
        }   // mat1/2/3 released here

        // 4. Age out the graveyard.
        for (uint32_t i = 0; i < Rhi::kMaxFramesInFlight; ++i)
        {
            resources.FlushGraveyard();
        }

        Core::Log::Info("[Test] Material set passed");
    }
}

int main()
{
    try
    {
        TestTexture();
        TestMaterialSet();

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
