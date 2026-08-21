#include "application.h"

#include "core/window.h"
#include "core/input.h"
#include "core/time.h"
#include "scene/scene.h"
#include "scene/asset_loader.h"
#include "scene/camera_controller.h"
#include "ui/ui.h"
#include "render/renderer.h"

#include <filesystem>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace
    {
        void InitScene(Scene& scene)
        {
            Kita::Pbrv::Mesh& mesh = scene.GetMesh();
            Kita::Pbrv::AssetLoader::LoadGltfMesh("assets/models/Cerberus_LP.glb", mesh);

            Kita::Pbrv::Material& mat = scene.GetMaterial();
            Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_A.tga", TextureType::Albedo, mat.GetAlbedoTex());
            Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_N.tga", TextureType::Normal, mat.GetNormalTex());
            Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_M.tga", TextureType::Linear, mat.GetMetallicTex());
            Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_R.tga", TextureType::Linear, mat.GetRoughnessTex());
            Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_AO.tga", TextureType::Linear, mat.GetAOTex());

            Kita::Pbrv::Skybox& skybox = scene.GetSkybox();
            Kita::Pbrv::AssetLoader::LoadSkybox("assets/textures/qwantani_moon_noon_puresky_4k.hdr", skybox);

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

        m_window = std::make_unique<Window>(800, 600, "Vk Pbr Viewer");
        m_input = std::make_unique<Input>(*m_window);
        m_time = std::make_unique<Time>();

        m_scene = std::make_unique<Scene>();
        InitScene(*m_scene);

        m_renderer = std::make_unique<Renderer>(*m_window);

        m_ui = std::make_unique<UI>(*m_scene);
    }

    Application::~Application() = default;

    void Application::Run()
    {
        CameraController cameraController(*m_input, m_scene->GetCamera());

        while (!m_window->ShouldClose())
        {
            m_window->PollEvents();
            m_input->Update();
            m_time->Update();

            float deltaTime = m_time->GetDeltaTime();

            if (m_input->IsKeyPressed(Key::Escape))
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
