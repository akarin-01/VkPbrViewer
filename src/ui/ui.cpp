#include "ui.h"

#include <glm/glm.hpp>

#include "imgui.h"
#include "scene/scene.h"

namespace Kita::Pbrv
{
    namespace
    {
        void UpdateStatsPanel(float deltaTime)
        {
            if (ImGui::CollapsingHeader("Stats"))
            {
                ImGui::Text("FPS      : %.1f", 1.0f / deltaTime);
                ImGui::Text("Frame ms : %.2f", deltaTime * 1000.0f);
            }
        }

        void UpdateEnvironmentPanel(Light& light)
        {
            if (ImGui::CollapsingHeader("Environment"))
            {
                ImGui::BeginChild("##LightBox", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
                {
                    ImGui::Text("Light");

                    glm::vec3 dir = light.GetDirection();
                    if (ImGui::DragFloat3("Direction", &dir.x, 0.05f, -1.0f, 1.0f))
                    {
                        light.SetDirection(dir);
                    }

                    glm::vec3 color = light.GetColor();
                    if (ImGui::ColorEdit3("Color", &color.x))
                    {
                        light.SetColor(color);
                    }

                    float intensity = light.GetIntensity();
                    if (ImGui::DragFloat("Intensity", &intensity, 0.05f, 0.0f, 100.0f))
                    {
                        light.SetIntensity(intensity);
                    }
                }
                ImGui::EndChild();
            }
        }

        void UpdateMaterialPanel(Material& mat)
        {
            if (ImGui::CollapsingHeader("Material"))
            {
                glm::vec4 albedo = mat.GetAlbedo();
                if (ImGui::ColorEdit4("Albedo", &albedo.x))
                {
                    mat.SetAlbedo(albedo);
                }

                float metallic = mat.GetMetallic();
                if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
                {
                    mat.SetMetallic(metallic);
                }

                float roughness = mat.GetRoughness();
                if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
                {
                    mat.SetRoughness(roughness);
                }

                float ao = mat.GetAO();
                if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f))
                {
                    mat.SetAO(ao);
                }
            }
        }

        void UpdatePostProcessPanel(PostProcess& postProcess)
        {
            if (ImGui::CollapsingHeader("Post Process"))
            {
                float ev = postProcess.GetEV();
                if (ImGui::SliderFloat("EV", &ev, -10.0f, 10.0f))
                {
                    postProcess.SetEV(ev);
                }
            }
        }
    }

    UI::UI(Scene& scene)
        : m_scene(scene)
    {
    }

    UI::~UI() = default;

    void UI::Update(float deltaTime)
    {
        DrawPanel(deltaTime);
    }

    bool UI::IsMouseHovered() const
    {
        return ImGui::GetIO().WantCaptureMouse;
    }

    void UI::DrawPanel(float deltaTime)
    {
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);

        if (ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove))
        {
            UpdateEnvironmentPanel(m_scene.GetLight());
            UpdateMaterialPanel(m_scene.GetMaterial());
            UpdatePostProcessPanel(m_scene.GetPostProcess());
            UpdateStatsPanel(deltaTime);
        }
        ImGui::End();
    }
}

