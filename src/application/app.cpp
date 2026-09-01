#include "app.h"

#include "core/input.h"
#include "core/time.h"
#include "core/window.h"
#include "resource/asset_manager.h"
#include "render/renderer.h"
#include "scene/scene.h"
#include "application/orbit_camera_controller.h"
#include "application/ui.h"

#include <filesystem>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Application
    {
        namespace
        {
            void InitScene(Scene::Scene& scene, Resource::AssetManager& assetMgr)
            {
                auto& object = scene.CreateObject();
                object.SetMesh(assetMgr.LoadMesh("assets/models/DamagedHelmet.gltf"));

                auto& mat = object.GetMaterial();
                mat.SetAlbedoTex(assetMgr.LoadTexture("assets/models/Default_albedo.jpg", Resource::TextureAsset::Type::Srgb));
                mat.SetNormalTex(assetMgr.LoadTexture("assets/models/Default_normal.jpg", Resource::TextureAsset::Type::Normal));
                mat.SetMRTex(assetMgr.LoadTexture("assets/models/Default_metalRoughness.jpg", Resource::TextureAsset::Type::MetallicRoughness));
                mat.SetAOTex(assetMgr.LoadTexture("assets/models/Default_AO.jpg", Resource::TextureAsset::Type::Linear));
                mat.SetEmissiveTex(assetMgr.LoadTexture("assets/models/Default_Emissive.jpg", Resource::TextureAsset::Type::Srgb));

                auto& skybox = scene.GetSkybox();
                skybox.SetSkybox(assetMgr.LoadTexture("assets/hdr/qwantani_moon_noon_puresky_4k.hdr", Resource::TextureAsset::Type::Hdr));

                scene.GetPostProcess().SetEV(-1.0f);
            }
        }

        App::App()
        {
            // Check if the "assets" directory exists
            if (!std::filesystem::is_directory("assets"))
            {
                throw std::runtime_error("assets/ directory not found");
            }

            m_window = std::make_unique<Core::Window>(800, 600, "Vk Pbr Viewer");
            m_input = std::make_unique<Core::Input>(*m_window);
            m_time = std::make_unique<Core::Time>();

            m_assetManager = std::make_unique<Resource::AssetManager>();
            m_renderer = std::make_unique<Render::Renderer>(*m_window, *m_assetManager);
            m_scene = std::make_unique<Scene::Scene>();
            InitScene(*m_scene, *m_assetManager);

            m_ui = std::make_unique<UI>(*m_scene, *m_assetManager);
        }

        App::~App() = default;

        void App::Run()
        {
            Application::OrbitCameraController cameraController(*m_input, m_scene->GetCamera());

            while (!m_window->ShouldClose())
            {
                m_window->PollEvents();
                m_input->Update();
                m_time->Update();

                float deltaTime = m_time->GetDeltaTime();

                if (m_input->IsKeyPressed(Core::Key::Escape))
                {
                    m_window->RequestClose();
                    continue;
                }

                m_renderer->NewFrame();

                if (!m_ui->IsMouseHovered())
                {
                    cameraController.Update(deltaTime);
                }

                m_ui->Update(deltaTime);

                m_scene->Update();
                m_renderer->DrawFrame();
            }
        }
    }
}
