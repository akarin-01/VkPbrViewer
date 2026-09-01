#include "ui.h"

#include "imgui.h"
#include "core/log.h"
#include "resource/asset_manager.h"
#include "resource/constants.h"
#include "scene/scene.h"
#include "application/ui_utils.h"

#include <glm/glm.hpp>
#include <string>

namespace Kita::Pbrv
{
    namespace Application
    {
        namespace
        {
            constexpr float kDragSpeed = 0.005f;
            constexpr float kAngleDragSpeed = 0.1f;

            void DrawTextureRaw(const char* title, Scene::Material& mat, Resource::MaterialSlot slot,
                Resource::TextureAsset::Type type, Resource::AssetManager& assets)
            {
                const auto tex = mat.GetTexture(slot);

                ImGui::TextUnformatted(title);

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
                if (ImGui::Button("Select"))
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

            void DrawLight(Scene::Light& light)
            {
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

            void DrawCamera(Scene::Camera& camera)
            {
                DrawBox("##CameraBox", "Camera", [&camera]()
                    {
                        // Position is derived by the camera controller each frame.
                        const glm::vec3 position = camera.GetPosition();
                        ImGui::Text("Position: X:%.2f Y:%.2f Z:%.2f", position.x, position.y, position.z);
                        ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.GetYaw(), camera.GetPitch());

                        float fov = camera.GetFov();
                        if (ImGui::SliderFloat("FOV", &fov, 30.0f, 90.0f))
                        {
                            camera.SetView(fov, camera.GetNear(), camera.GetFar());
                        }
                    });
            }

            void DrawSkybox(Scene::Skybox& skybox, Resource::AssetManager& assets)
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

                        if (ImGui::Button("Select##Skybox"))
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
            }

            void DrawTransform(Scene::Object& object)
            {
                DrawBox("##TransformBox", "Transform", [&object]()
                    {
                        glm::vec3 position = object.GetPosition();
                        if (ImGui::DragFloat3("Position", &position.x, kDragSpeed))
                        {
                            object.SetPosition(position);
                        }

                        glm::vec3 rotation = object.GetRotation();
                        if (ImGui::DragFloat3("Rotation", &rotation.x, kAngleDragSpeed))
                        {
                            object.SetRotation(rotation);
                        }

                        glm::vec3 scale = object.GetScale();
                        if (ImGui::DragFloat3("Scale", &scale.x, kDragSpeed))
                        {
                            object.SetScale(scale);
                        }
                    });
            }

            void DrawMesh(Scene::Object& object, Resource::AssetManager& assets)
            {
                DrawBox("##MeshBox", "Mesh", [&object, &assets]()
                    {
                        const auto& mesh = object.GetMesh();
                        if (mesh.IsValid())
                        {
                            ImGui::TextUnformatted(mesh->m_name.c_str());
                            ImGui::SameLine();
                            ImGui::Text("(%zu verts, %zu idx)", mesh->GetVertexCount(), mesh->GetIndexCount());
                        }
                        else
                        {
                            ImGui::TextUnformatted("empty");
                        }

                        if (ImGui::Button("Select##Mesh"))
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
            }

            void DrawMaterial(Scene::Material& mat, Resource::AssetManager& assets)
            {
                DrawBox("##MaterialBox", "Material", [&mat, &assets]()
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

                                float emissiveIntensity = mat.GetEmissiveIntensity();
                                if (ImGui::DragFloat("Emissive Intensity", &emissiveIntensity, kDragSpeed, 0.0f, 10.0f))
                                {
                                    mat.SetEmissiveIntensity(emissiveIntensity);
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

            void DrawPostProcess(Scene::PostProcess& postProcess)
            {
                DrawBox("##PostProcessBox", "Post Process", [&postProcess]()
                    {
                        float ev = postProcess.GetEV();
                        if (ImGui::SliderFloat("EV", &ev, -10.0f, 10.0f))
                        {
                            postProcess.SetEV(ev);
                        }
                    });
            }

            void DrawStatsPanel(float deltaTime)
            {
                if (ImGui::CollapsingHeader("Stats"))
                {
                    ImGui::Text("FPS      : %.1f", 1.0f / deltaTime);
                    ImGui::Text("Frame ms : %.2f", deltaTime * 1000.0f);
                }
            }

            void DrawEnvironmentPanel(Scene::Camera& camera, Scene::Light& light,
                Scene::Skybox& skybox, Resource::AssetManager& assets)
            {
                if (ImGui::CollapsingHeader("Environment"))
                {
                    DrawSkybox(skybox, assets);
                    DrawLight(light);
                    DrawCamera(camera);
                }
            }

            void DrawObjectsPanel(Scene::Scene& scene, Resource::AssetManager& assets)
            {
                if (ImGui::CollapsingHeader("Objects"))
                {
                    ImGui::Indent();

                    for (auto& object : scene.GetObjects())
                    {
                        ImGui::PushID(static_cast<int>(object.GetId()));

                        const std::string title = "Object " + std::to_string(object.GetId());
                        if (ImGui::CollapsingHeader(title.c_str()))
                        {
                            // Delete the object as the first row
                            if (ImGui::Button("Delete"))
                            {
                                scene.DestroyObject(object.GetId());
                            }

                            DrawTransform(object);
                            DrawMesh(object, assets);
                            DrawMaterial(object.GetMaterial(), assets);
                        }

                        ImGui::PopID();
                    }

                    // Create a new object below the list
                    if (ImGui::Button("Create Object"))
                    {
                        scene.CreateObject();
                    }

                    ImGui::Unindent();
                }
            }

            void DrawPostProcessPanel(Scene::PostProcess& postProcess)
            {
                if (ImGui::CollapsingHeader("Post Process"))
                {
                    DrawPostProcess(postProcess);
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
                DrawEnvironmentPanel(m_scene.GetCamera(), m_scene.GetLight(), m_scene.GetSkybox(), m_assets);
                DrawObjectsPanel(m_scene, m_assets);
                DrawPostProcessPanel(m_scene.GetPostProcess());
                DrawStatsPanel(deltaTime);
            }
            ImGui::End();
        }
    }
}
