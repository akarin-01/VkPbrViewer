#include "application.h"
#include "core/input.h"
#include "core/time.h"
#include "core/window.h"
#include "render/renderer.h"
#include "scene/asset_loader.h"
#include "scene/camera_controller.h"
#include "scene/scene.h"
#include "application/ui.h"

#include <filesystem>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Application
    {
        namespace
        {
            void InitScene(Scene::Scene& scene)
            {
                Kita::Pbrv::Scene::Mesh& mesh = scene.GetMesh();
                Kita::Pbrv::Scene::AssetLoader::LoadGltfMesh("assets/models/DamagedHelmet.gltf", mesh);

                Kita::Pbrv::Scene::Material& mat = scene.GetMaterial();
                Kita::Pbrv::Scene::AssetLoader::LoadTexture("assets/models/Default_albedo.jpg", Scene::TextureType::Srgb, mat.GetAlbedoTex());
                Kita::Pbrv::Scene::AssetLoader::LoadTexture("assets/models/Default_normal.jpg", Scene::TextureType::Normal, mat.GetNormalTex());
                Kita::Pbrv::Scene::AssetLoader::LoadTexture("assets/models/Default_metalRoughness.jpg", Scene::TextureType::MetallicRoughness, mat.GetMRTex());
                Kita::Pbrv::Scene::AssetLoader::LoadTexture("assets/models/Default_AO.jpg", Scene::TextureType::Linear, mat.GetAOTex());
                Kita::Pbrv::Scene::AssetLoader::LoadTexture("assets/models/Default_Emissive.jpg", Scene::TextureType::Srgb, mat.GetEmissiveTex());

                Kita::Pbrv::Scene::Skybox& skybox = scene.GetSkybox();
                Kita::Pbrv::Scene::AssetLoader::LoadSkybox("assets/hdr/qwantani_moon_noon_puresky_4k.hdr", skybox);

                scene.GetPostProcess().SetEV(-1.0f);
            }
        }

        Application::Application()
        {
            // Check if the "assets" directory exists
            if (!std::filesystem::is_directory("assets"))
            {
                throw std::runtime_error("assets/ directory not found");
            }

            m_window = std::make_unique<Core::Window>(800, 600, "Vk Pbr Viewer");
            m_input = std::make_unique<Core::Input>(*m_window);
            m_time = std::make_unique<Core::Time>();

            m_scene = std::make_unique<Scene::Scene>();
            InitScene(*m_scene);

            m_renderer = std::make_unique<Render::Renderer>(*m_window);

            m_ui = std::make_unique<Ui::UI>(*m_scene);
        }

        Application::~Application() = default;

        void Application::Run()
        {
            Scene::CameraController cameraController(*m_input, m_scene->GetCamera());

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
                m_renderer->DrawFrame(*m_scene);
            }
        }
    }
}
