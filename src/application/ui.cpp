#include "ui.h"

#include "imgui.h"
#include "application/ui_utils.h"
#include "scene/scene.h"
#include "scene/asset_loader.h"

#include <glm/glm.hpp>
#include <array>

namespace Kita::Pbrv
{
    namespace Ui
    {
        namespace
        {
            constexpr float kDragSpeed = 0.005f;

            void DrawTextureRaw(const char* title, Scene::Texture& tex, Scene::TextureType type)
            {
                ImGui::TextUnformatted(title);
                ImGui::SameLine();
                ImGui::TextUnformatted(tex.GetName().c_str());
                ImGui::SameLine();
                ImGui::Text("(%ux%u)", tex.GetWidth(), tex.GetHeight());

                ImGui::PushID(title);
                if (ImGui::Button("Open"))
                {
                    auto path = OpenFileDialog("Scene::Texture Files\0*.jpg;*.png;*.tga\0All Files\0*.*\0");
                    if (path)
                    {
                        Scene::AssetLoader::LoadTexture(path.value(), type, tex);
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

            void DrawEnvironmentPanel(Scene::Skybox& skybox, Scene::Light& light)
            {
                if (ImGui::CollapsingHeader("Environment"))
                {
                    DrawBox("##SkyboxBox", "Scene::Skybox", [&skybox]()
                        {
                            ImGui::TextUnformatted(skybox.GetName().c_str());
                            ImGui::SameLine();
                            ImGui::Text("(%ux%u)", skybox.GetWidth(), skybox.GetHeight());

                            if (ImGui::Button("Open##Scene::Skybox"))
                            {
                                auto path = OpenFileDialog("Equirect Files\0*.hdr\0All Files\0*.*\0");
                                if (path)
                                {
                                    Scene::AssetLoader::LoadSkybox(path.value(), skybox);
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Delete##Scene::Skybox"))
                            {
                                skybox.SetEmpty();
                            }
                        });

                    DrawBox("##LightBox", "Scene::Light", [&light]()
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

            void DrawObjectPanel(Scene::Mesh& mesh, Scene::Material& mat)
            {
                if (ImGui::CollapsingHeader("Object"))
                {
                    DrawBox("##MeshBox", "Scene::Mesh", [&mesh]()
                        {
                            ImGui::TextUnformatted(mesh.GetName().c_str());
                            ImGui::Text("%zu vertices, %zu indices", mesh.GetVertexCount(), mesh.GetIndexCount());

                            if (ImGui::Button("Open##Scene::Mesh"))
                            {
                                auto path = OpenFileDialog("Scene::Mesh Files\0*.glb;*.gltf\0All Files\0*.*\0");
                                if (path)
                                {
                                    Scene::AssetLoader::LoadGltfMesh(path.value(), mesh);
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Delete##Scene::Mesh"))
                            {
                                mesh.SetEmpty();
                            }
                        });

                    DrawBox("##MaterialBox", "Scene::Material", [&mat]()
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

                                    glm::vec3 emissive = mat.GetEmissive();
                                    if (ImGui::ColorEdit3("Emissive", &emissive.x))
                                    {
                                        mat.SetEmissive(emissive);
                                    }
                                });

                            DrawBox("##TexturesBox", "Textures", [&mat]()
                                {
                                    DrawTextureRaw("Albedo:   ", mat.GetAlbedoTex(), Scene::TextureType::Srgb);
                                    DrawTextureRaw("Normal:   ", mat.GetNormalTex(), Scene::TextureType::Normal);
                                    DrawTextureRaw("MR:       ", mat.GetMRTex(), Scene::TextureType::MetallicRoughness);
                                    DrawTextureRaw("AO:       ", mat.GetAOTex(), Scene::TextureType::Linear);
                                    DrawTextureRaw("Emissive: ", mat.GetEmissiveTex(), Scene::TextureType::Srgb);
                                });
                        });
                }
            }

            void DrawPostProcessPanel(Scene::PostProcess& postProcess)
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

        UI::UI(Scene::Scene& scene)
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
}

