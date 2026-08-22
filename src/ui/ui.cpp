#include "ui.h"

#include "imgui.h"
#include "ui/ui_utils.h"
#include "scene/scene.h"
#include "scene/asset_loader.h"

#include <glm/glm.hpp>
#include <array>

namespace Kita::Pbrv
{
    namespace
    {
        constexpr float kDragSpeed = 0.005f;

        void DrawTextureRaw(const char* title, Texture& tex, TextureType type)
        {
            ImGui::TextUnformatted(title);
            ImGui::SameLine();
            ImGui::TextUnformatted(tex.GetName().c_str());
            ImGui::SameLine();
            ImGui::Text("(%ux%u)", tex.GetWidth(), tex.GetHeight());

            ImGui::PushID(title);
            if (ImGui::Button("Open"))
            {
                auto path = OpenFileDialog("Texture Files\0*.jpg;*.png;*.tga\0All Files\0*.*\0");
                if (path)
                {
                    AssetLoader::LoadTexture(path.value(), type, tex);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete"))
            {
                tex.SetEmpty();
            }
            ImGui::PopID();
        }

        void DrawStatsPanel(float deltaTime)
        {
            if (ImGui::CollapsingHeader("Stats"))
            {
                ImGui::Text("FPS      : %.1f", 1.0f / deltaTime);
                ImGui::Text("Frame ms : %.2f", deltaTime * 1000.0f);
            }
        }

        void DrawEnvironmentPanel(Skybox& skybox, Light& light)
        {
            if (ImGui::CollapsingHeader("Environment"))
            {
                DrawBox("##SkyboxBox", "Skybox", [&skybox]()
                    {
                        ImGui::TextUnformatted(skybox.GetName().c_str());
                        ImGui::SameLine();
                        ImGui::Text("(%ux%u)", skybox.GetWidth(), skybox.GetHeight());

                        if (ImGui::Button("Open##Skybox"))
                        {
                            auto path = OpenFileDialog("Equirect Files\0*.hdr\0All Files\0*.*\0");
                            if (path)
                            {
                                AssetLoader::LoadSkybox(path.value(), skybox);
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Delete##Skybox"))
                        {
                            skybox.SetEmpty();
                        }
                    });

                DrawBox("##LightBox", "Light", [&light]()
                    {
                        glm::vec3 pos = light.GetPosition();
                        if (ImGui::DragFloat3("Position", &pos.x, kDragSpeed))
                        {
                            light.SetPosition(pos);
                        }

                        glm::vec3 color = light.GetColor();
                        if (ImGui::ColorEdit3("Color", &color.x))
                        {
                            light.SetColor(color);
                        }

                        float intensity = light.GetIntensity();
                        if (ImGui::DragFloat("Intensity", &intensity, kDragSpeed, 0.0f, 100.0f))
                        {
                            light.SetIntensity(intensity);
                        }
                    });
            }
        }

        void DrawObjectPanel(Mesh& mesh, Material& mat)
        {
            if (ImGui::CollapsingHeader("Object"))
            {
                DrawBox("##MeshBox", "Mesh", [&mesh]()
                    {
                        ImGui::TextUnformatted(mesh.GetName().c_str());
                        ImGui::Text("%zu vertices, %zu indices", mesh.GetVertexCount(), mesh.GetIndexCount());

                        if (ImGui::Button("Open##Mesh"))
                        {
                            auto path = OpenFileDialog("Mesh Files\0*.glb;*.gltf\0All Files\0*.*\0");
                            if (path)
                            {
                                AssetLoader::LoadGltfMesh(path.value(), mesh);
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Delete##Mesh"))
                        {
                            mesh.SetEmpty();
                        }
                    });

                DrawBox("##MaterialBox", "Material", [&mat]()
                    {
                        DrawBox("##ParamsBox", "Params", [&mat]()
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
                            });

                        DrawBox("##TexturesBox", "Textures", [&mat]()
                            {
                                DrawTextureRaw("Albedo:   ", mat.GetAlbedoTex(), TextureType::Albedo);
                                DrawTextureRaw("Normal:   ", mat.GetNormalTex(), TextureType::Normal);
                                DrawTextureRaw("Metallic: ", mat.GetMetallicTex(), TextureType::Linear);
                                DrawTextureRaw("Roughness:", mat.GetRoughnessTex(), TextureType::Linear);
                                DrawTextureRaw("AO:       ", mat.GetAOTex(), TextureType::Linear);
                            });
                    });
            }
        }

        void DrawPostProcessPanel(PostProcess& postProcess)
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
            DrawEnvironmentPanel(m_scene.GetSkybox(), m_scene.GetLight());
            DrawObjectPanel(m_scene.GetMesh(), m_scene.GetMaterial());
            DrawPostProcessPanel(m_scene.GetPostProcess());
            DrawStatsPanel(deltaTime);
        }
        ImGui::End();
    }
}

