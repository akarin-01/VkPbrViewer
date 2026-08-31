#include "ui.h"

#include "imgui.h"
#include "core/log.h"
#include "resource/asset_manager.h"
#include "resource/constants.h"
#include "scene/scene.h"
#include "application/ui_utils.h"

#include <glm/glm.hpp>
#include <array>

namespace Kita::Pbrv
{
    namespace Application
    {
        namespace
        {
            constexpr float kDragSpeed = 0.005f;

            void DrawTextureRaw(const char* title, Scene::Material& mat, Resource::MaterialSlot slot,
                Resource::TextureAsset::Type type, Resource::AssetManager& assets)
            {
                const auto tex = mat.GetTexture(slot);

                ImGui::TextUnformatted(title);
                ImGui::SameLine();
                if (tex.IsValid())
                {
                    ImGui::TextUnformatted(tex->m_name.c_str());
                    ImGui::SameLine();
                    ImGui::Text("(%ux%u)", tex->m_width, tex->m_height);
                }
                else
                {
                    ImGui::TextUnformatted("empty");
                }

                ImGui::PushID(title);
                if (ImGui::Button("Open"))
                {
                    auto path = OpenFileDialog("TextureAsset Files\0*.jpg;*.png;*.tga\0All Files\0*.*\0");
                    if (path)
                    {
                        try
                        {
                            mat.SetTexture(slot, assets.LoadTexture(path.value(), type));
                        }
                        catch (const std::exception& e)
                        {
                            Core::Log::Error("[UI] ", e.what());
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete"))
                {
                    mat.SetTexture(slot, {});
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

            void DrawEnvironmentPanel(Scene::Skybox& skybox, Scene::Light& light, Resource::AssetManager& assets)
            {
                if (ImGui::CollapsingHeader("Environment"))
                {
                    DrawBox("##SkyboxBox", "Skybox", [&skybox, &assets]()
                        {
                            const auto& tex = skybox.GetSkybox();
                            if (tex.IsValid())
                            {
                                ImGui::TextUnformatted(tex->m_name.c_str());
                                ImGui::SameLine();
                                ImGui::Text("(%ux%u)", tex->m_width, tex->m_height);
                            }
                            else
                            {
                                ImGui::TextUnformatted("empty");
                            }

                            if (ImGui::Button("Open##Skybox"))
                            {
                                auto path = OpenFileDialog("Equirect Files\0*.hdr\0All Files\0*.*\0");
                                if (path)
                                {
                                    try
                                    {
                                        skybox.SetSkybox(assets.LoadTexture(path.value(), Resource::TextureAsset::Type::Hdr));
                                    }
                                    catch (const std::exception& e)
                                    {
                                        Core::Log::Error("[UI] ", e.what());
                                    }
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Delete##Skybox"))
                            {
                                skybox.SetSkybox(Resource::TextureAsset::Handle{});
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

            void DrawObjectPanel(Scene::Object& object, Resource::AssetManager& assets)
            {
                if (ImGui::CollapsingHeader("Object"))
                {
                    DrawBox("##MeshBox", "Mesh", [&object, &assets]()
                        {
                            const auto& mesh = object.GetMesh();
                            if (mesh.IsValid())
                            {
                                ImGui::TextUnformatted(mesh->m_name.c_str());
                                ImGui::Text("%zu vertices, %zu indices", mesh->GetVertexCount(), mesh->GetIndexCount());
                            }
                            else
                            {
                                ImGui::TextUnformatted("empty");
                            }

                            if (ImGui::Button("Open##Mesh"))
                            {
                                auto path = OpenFileDialog("Mesh Files\0*.glb;*.gltf\0All Files\0*.*\0");
                                if (path)
                                {
                                    try
                                    {
                                        object.SetMesh(assets.LoadMesh(path.value()));
                                    }
                                    catch (const std::exception& e)
                                    {
                                        Core::Log::Error("[UI] ", e.what());
                                    }
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Delete##Mesh"))
                            {
                                object.SetMesh(Resource::MeshAsset::Handle{});
                            }
                        });

                    DrawBox("##MaterialBox", "Material", [&object, &assets]()
                        {
                            auto& mat = object.GetMaterial();

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

                            DrawBox("##TexturesBox", "Textures", [&mat, &assets]()
                                {
                                    DrawTextureRaw("Albedo:   ", mat, Resource::MaterialSlot::Albedo, Resource::TextureAsset::Type::Srgb, assets);
                                    DrawTextureRaw("Normal:   ", mat, Resource::MaterialSlot::Normal, Resource::TextureAsset::Type::Normal, assets);
                                    DrawTextureRaw("MR:       ", mat, Resource::MaterialSlot::MetallicRoughness, Resource::TextureAsset::Type::MetallicRoughness, assets);
                                    DrawTextureRaw("AO:       ", mat, Resource::MaterialSlot::AO, Resource::TextureAsset::Type::Linear, assets);
                                    DrawTextureRaw("Emissive: ", mat, Resource::MaterialSlot::Emissive, Resource::TextureAsset::Type::Srgb, assets);
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

        UI::UI(Scene::Scene& scene, Resource::AssetManager& assets)
            : m_scene(scene),
            m_assets(assets)
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
                DrawEnvironmentPanel(m_scene.GetSkybox(), m_scene.GetLight(), m_assets);
                DrawObjectPanel(m_scene.GetObject(), m_assets);
                DrawPostProcessPanel(m_scene.GetPostProcess());
                DrawStatsPanel(deltaTime);
            }
            ImGui::End();
        }
    }
}
