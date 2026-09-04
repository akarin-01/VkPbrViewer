#include "asset_utils.h"

#include "core/log.h"
#include "core/path.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <optional>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace
        {
            int GetTextureChannels(TextureAsset::Type type)
            {
                switch (type)
                {
                case TextureAsset::Type::Srgb:                 return 4;
                case TextureAsset::Type::Normal:               return 4;
                case TextureAsset::Type::MetallicRoughness:    return 4;
                case TextureAsset::Type::Linear:               return 1;
                case TextureAsset::Type::Hdr:                  return 4;
                default: throw std::runtime_error("Invalid texture type!");
                }
            }

            size_t GetBytesPerPixel(TextureAsset::Type type)
            {
                switch (type)
                {
                case TextureAsset::Type::Srgb:                 return 4;
                case TextureAsset::Type::Normal:               return 4;
                case TextureAsset::Type::MetallicRoughness:    return 4;
                case TextureAsset::Type::Linear:               return 1;
                case TextureAsset::Type::Hdr:                  return 4 * sizeof(float);
                default: throw std::runtime_error("Invalid texture type!");
                }
            }

            glm::vec4 ReadAccessorElement(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index)
            {
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const uint8_t* src = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset
                    + accessor.ByteStride(bufferView) * index;

                int numComponents = tinygltf::GetNumComponentsInType(accessor.type);
                if (numComponents > 4)
                {
                    numComponents = 4;
                }

                glm::vec4 result{ 0.0f };
                for (int i = 0; i < numComponents; ++i)
                {
                    switch (accessor.componentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        result[i] = static_cast<float>(src[i]) / 255.0f;
                        break;
                    case TINYGLTF_COMPONENT_TYPE_BYTE:
                        result[i] = static_cast<float>(reinterpret_cast<const int8_t*>(src)[i]) / 127.0f;
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        result[i] = static_cast<float>(reinterpret_cast<const uint16_t*>(src)[i]) / 65535.0f;
                        break;
                    case TINYGLTF_COMPONENT_TYPE_SHORT:
                        result[i] = static_cast<float>(reinterpret_cast<const int16_t*>(src)[i]) / 32767.0f;
                        break;
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        result[i] = reinterpret_cast<const float*>(src)[i];
                        break;
                    default:
                        throw std::runtime_error("Unsupported accessor component type!");
                    }
                }

                return result;
            }

            uint32_t ReadAccessorIndex(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index)
            {
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const uint8_t* src = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset
                    + accessor.ByteStride(bufferView) * index;

                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    return src[0];
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    return reinterpret_cast<const uint16_t*>(src)[0];
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    return reinterpret_cast<const uint32_t*>(src)[0];
                default:
                    throw std::runtime_error("Unsupported index component type!");
                }
            }

            void ComputeTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
            {
                if (indices.size() % 3 != 0)
                {
                    Core::Log::Warning("[Resource] Index count is not a multiple of 3, skipping tangent computation");
                    return;
                }

                std::vector<glm::vec3> tan1(vertices.size(), glm::vec3(0.0f));
                std::vector<glm::vec3> tan2(vertices.size(), glm::vec3(0.0f));

                for (size_t i = 0; i + 2 < indices.size(); i += 3)
                {
                    const uint32_t i0 = indices[i + 0];
                    const uint32_t i1 = indices[i + 1];
                    const uint32_t i2 = indices[i + 2];

                    const glm::vec3& p0 = vertices[i0].position;
                    const glm::vec3& p1 = vertices[i1].position;
                    const glm::vec3& p2 = vertices[i2].position;

                    const glm::vec2& uv0 = vertices[i0].texCoord;
                    const glm::vec2& uv1 = vertices[i1].texCoord;
                    const glm::vec2& uv2 = vertices[i2].texCoord;

                    const glm::vec3 e1 = p1 - p0;
                    const glm::vec3 e2 = p2 - p0;
                    const glm::vec2 duv1 = uv1 - uv0;
                    const glm::vec2 duv2 = uv2 - uv0;

                    const float r = duv1.x * duv2.y - duv2.x * duv1.y;
                    if (std::abs(r) < 1e-8f)
                    {
                        // Degenerate UVs, skip this triangle
                        continue;
                    }

                    const float f = 1.0f / r;
                    const glm::vec3 tangent = (e1 * duv2.y - e2 * duv1.y) * f;
                    const glm::vec3 bitangent = (e2 * duv1.x - e1 * duv2.x) * f;

                    tan1[i0] += tangent;
                    tan1[i1] += tangent;
                    tan1[i2] += tangent;

                    tan2[i0] += bitangent;
                    tan2[i1] += bitangent;
                    tan2[i2] += bitangent;
                }

                for (size_t i = 0; i < vertices.size(); ++i)
                {
                    if (vertices[i].tangent != glm::vec4(0.0f))
                    {
                        // Has tangent, skip this vertex
                        continue;
                    }

                    const glm::vec3& n = vertices[i].normal;

                    glm::vec3 t = tan1[i] - n * glm::dot(n, tan1[i]);   // Gram-Schmidt orthogonalization
                    t = glm::length(t) > 1e-8f ? glm::normalize(t) : glm::vec3(1.0f, 0.0f, 0.0f);

                    // w = handedness, used by the shader to derive the bitangent direction
                    const float w = glm::dot(glm::cross(n, t), tan2[i]) < 0.0f ? -1.0f : 1.0f;

                    vertices[i].tangent = glm::vec4(t, w);
                }
            }

            glm::mat4 MakeNodeLocalMatrix(const tinygltf::Node& node)
            {
                if (node.matrix.size() >= 16)
                {
                    return glm::make_mat4(node.matrix.data());
                }

                const glm::vec3 translation = node.translation.size() == 3
                    ? glm::vec3(
                        static_cast<float>(node.translation[0]),
                        static_cast<float>(node.translation[1]),
                        static_cast<float>(node.translation[2]))
                    : glm::vec3(0.0f);

                const glm::quat rotation = node.rotation.size() == 4
                    ? glm::quat(
                        static_cast<float>(node.rotation[3]),
                        static_cast<float>(node.rotation[0]),
                        static_cast<float>(node.rotation[1]),
                        static_cast<float>(node.rotation[2]))
                    : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

                const glm::vec3 scale = node.scale.size() == 3
                    ? glm::vec3(
                        static_cast<float>(node.scale[0]),
                        static_cast<float>(node.scale[1]),
                        static_cast<float>(node.scale[2]))
                    : glm::vec3(1.0f);

                return glm::translate(glm::mat4(1.0f), translation)
                    * glm::mat4_cast(rotation)
                    * glm::scale(glm::mat4(1.0f), scale);
            }

            bool FindFirstMeshWorld(const tinygltf::Model& model, int nodeIndex,
                int targetMeshIndex, const glm::mat4& parentWorld, glm::mat4& outWorld)
            {
                if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size()))
                {
                    return false;
                }

                const tinygltf::Node& node = model.nodes[nodeIndex];
                const glm::mat4 world = parentWorld * MakeNodeLocalMatrix(node);

                if (node.mesh == targetMeshIndex)
                {
                    outWorld = world;
                    return true;
                }

                for (int child : node.children)
                {
                    if (FindFirstMeshWorld(model, child, targetMeshIndex, world, outWorld))
                    {
                        return true;
                    }
                }

                return false;
            }

            Transform DecomposeTransform(const glm::mat4& matrix)
            {
                Transform transform;

                glm::vec3 scale;
                glm::quat rotation;
                glm::vec3 translation;
                glm::vec3 skew;
                glm::vec4 perspective;

                if (glm::decompose(matrix, scale, rotation, translation, skew, perspective))
                {
                    transform.m_position = translation;
                    transform.m_rotation = glm::degrees(glm::eulerAngles(rotation));
                    transform.m_scale = scale;
                }

                return transform;
            }

            MeshAsset BuildMeshFromPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& name)
            {
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;

                if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                {
                    throw std::runtime_error("Unsupported primitive mode");
                }

                auto positionIt = primitive.attributes.find("POSITION");
                if (positionIt == primitive.attributes.end())
                {
                    throw std::runtime_error("Primitive has no POSITION attribute");
                }
                const tinygltf::Accessor& positionAccessor = model.accessors[positionIt->second];

                auto normalIt = primitive.attributes.find("NORMAL");
                bool hasNormal = (normalIt != primitive.attributes.end());
                const tinygltf::Accessor* normalAccessor = hasNormal ? &model.accessors[normalIt->second] : nullptr;

                auto texCoordIt = primitive.attributes.find("TEXCOORD_0");
                bool hasTexCoord = (texCoordIt != primitive.attributes.end());
                const tinygltf::Accessor* texCoordAccessor = hasTexCoord ? &model.accessors[texCoordIt->second] : nullptr;

                auto tangentIt = primitive.attributes.find("TANGENT");
                bool hasTangent = (tangentIt != primitive.attributes.end());
                const tinygltf::Accessor* tangentAccessor = hasTangent ? &model.accessors[tangentIt->second] : nullptr;

                vertices.resize(positionAccessor.count);
                for (size_t i = 0; i < positionAccessor.count; ++i)
                {
                    Vertex& vertex = vertices[i];
                    vertex.position = ReadAccessorElement(model, positionAccessor, i);
                    vertex.normal = hasNormal ?
                        ReadAccessorElement(model, *normalAccessor, i) :
                        glm::vec3(0.0f, 1.0f, 0.0f);
                    vertex.texCoord = hasTexCoord ?
                        ReadAccessorElement(model, *texCoordAccessor, i) :
                        glm::vec2(0.0f);
                    vertex.tangent = hasTangent ?
                        ReadAccessorElement(model, *tangentAccessor, i) :
                        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                }

                if (primitive.indices < 0)
                {
                    indices.reserve(positionAccessor.count);
                    for (size_t i = 0; i < positionAccessor.count; ++i)
                    {
                        indices.push_back(static_cast<uint32_t>(i));
                    }
                }
                else
                {
                    const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                    indices.reserve(indexAccessor.count);
                    for (size_t i = 0; i < indexAccessor.count; ++i)
                    {
                        indices.push_back(ReadAccessorIndex(model, indexAccessor, i));
                    }
                }

                if (vertices.empty() || indices.empty())
                {
                    throw std::runtime_error("Primitive has no valid triangle data");
                }

                ComputeTangents(vertices, indices);

                MeshAsset mesh;
                mesh.m_name = name.empty() ? "mesh" : name;
                mesh.m_vertices = std::move(vertices);
                mesh.m_indices = std::move(indices);
                return mesh;
            }

            TextureKey MakeTextureKey(const tinygltf::Model& model, int textureIndex,
                const std::filesystem::path& baseDir, TextureAsset::Type type, const std::string& gltfPath)
            {
                TextureKey key;

                if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
                {
                    return key;
                }

                const tinygltf::Texture& texture = model.textures[textureIndex];
                if (texture.source < 0 || texture.source >= static_cast<int>(model.images.size()))
                {
                    return key;
                }

                const tinygltf::Image& image = model.images[texture.source];

                // Internal textures are intentionally not supported yet.
                if (image.uri.empty() || image.uri.rfind("data:", 0) == 0)
                {
                    Core::Log::Warning("[Resource] Skipping non-external texture in '", gltfPath, "'");
                    return key;
                }

                key.m_path = Core::Path::Normalize((baseDir / image.uri).string());
                key.m_type = type;
                return key;
            }

            void FillMaterial(const tinygltf::Model& model, ModelAsset::Part& part,
                const tinygltf::Material& material,
                const std::filesystem::path& baseDir, const std::string& gltfPath)
            {
                const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;

                if (pbr.baseColorFactor.size() >= 4)
                {
                    part.m_material.m_baseColorFactor = glm::vec4(
                        static_cast<float>(pbr.baseColorFactor[0]),
                        static_cast<float>(pbr.baseColorFactor[1]),
                        static_cast<float>(pbr.baseColorFactor[2]),
                        static_cast<float>(pbr.baseColorFactor[3]));
                }

                part.m_material.m_metallicFactor = static_cast<float>(pbr.metallicFactor);
                part.m_material.m_roughnessFactor = static_cast<float>(pbr.roughnessFactor);

                if (pbr.baseColorTexture.index >= 0)
                {
                    part.m_textureKeys[static_cast<size_t>(MaterialSlot::Albedo)] =
                        MakeTextureKey(model, pbr.baseColorTexture.index, baseDir, TextureAsset::Type::Srgb, gltfPath);
                }
                if (pbr.metallicRoughnessTexture.index >= 0)
                {
                    part.m_textureKeys[static_cast<size_t>(MaterialSlot::MetallicRoughness)] =
                        MakeTextureKey(model, pbr.metallicRoughnessTexture.index, baseDir,
                            TextureAsset::Type::MetallicRoughness, gltfPath);
                }

                if (material.normalTexture.index >= 0)
                {
                    part.m_textureKeys[static_cast<size_t>(MaterialSlot::Normal)] =
                        MakeTextureKey(model, material.normalTexture.index, baseDir, TextureAsset::Type::Normal, gltfPath);
                }
                if (material.occlusionTexture.index >= 0)
                {
                    part.m_textureKeys[static_cast<size_t>(MaterialSlot::AO)] =
                        MakeTextureKey(model, material.occlusionTexture.index, baseDir, TextureAsset::Type::Linear, gltfPath);
                }
                if (material.emissiveTexture.index >= 0)
                {
                    part.m_textureKeys[static_cast<size_t>(MaterialSlot::Emissive)] =
                        MakeTextureKey(model, material.emissiveTexture.index, baseDir, TextureAsset::Type::Srgb, gltfPath);
                }

                if (material.emissiveFactor.size() >= 3)
                {
                    part.m_material.m_emissiveFactor = glm::vec3(
                        static_cast<float>(material.emissiveFactor[0]),
                        static_cast<float>(material.emissiveFactor[1]),
                        static_cast<float>(material.emissiveFactor[2]));
                }
            }
        }

        namespace AssetUtils
        {
            ModelAsset AssetUtils::LoadGltf(const std::string& path)
            {
                Core::Log::Info("[Resource] Load glTF: ", path);

                tinygltf::Model model;
                tinygltf::TinyGLTF loader;
                std::string err;
                std::string warn;

                bool success = (std::filesystem::path(path).extension() == ".glb")
                    ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
                    : loader.LoadASCIIFromFile(&model, &err, &warn, path);

                if (!warn.empty())
                {
                    Core::Log::Warning("[Resource] glTF warning: ", warn);
                }
                if (!success)
                {
                    throw std::runtime_error("Failed to load glTF '" + path + "': " + err);
                }

                const std::filesystem::path filePath(path);
                const std::filesystem::path baseDir = filePath.parent_path();

                ModelAsset asset;
                asset.m_name = filePath.stem().string();

                std::vector<Transform> meshTransforms(model.meshes.size());
                if (!model.scenes.empty())
                {
                    const int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
                    if (sceneIndex < static_cast<int>(model.scenes.size()))
                    {
                        const auto& rootNodes = model.scenes[sceneIndex].nodes;
                        for (size_t meshIndex = 0; meshIndex < model.meshes.size(); ++meshIndex)
                        {
                            for (int root : rootNodes)
                            {
                                glm::mat4 worldMatrix(1.0f);
                                if (FindFirstMeshWorld(model, root, static_cast<int>(meshIndex),
                                    glm::mat4(1.0f), worldMatrix))
                                {
                                    meshTransforms[meshIndex] = DecomposeTransform(worldMatrix);
                                    break;
                                }
                            }
                        }
                    }
                }

                for (size_t meshIndex = 0; meshIndex < model.meshes.size(); ++meshIndex)
                {
                    const auto& mesh = model.meshes[meshIndex];

                    for (const auto& primitive : mesh.primitives)
                    {
                        if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                        {
                            Core::Log::Warning("[Resource] Unsupported primitive mode, skipped");
                            continue;
                        }

                        ModelAsset::Part part;
                        part.m_name = mesh.name.empty() ? filePath.stem().string() : mesh.name;
                        part.m_mesh = BuildMeshFromPrimitive(model, primitive, part.m_name);
                        part.m_transform = meshTransforms[meshIndex];

                        if (primitive.material >= 0
                            && primitive.material < static_cast<int>(model.materials.size()))
                        {
                            FillMaterial(model, part, model.materials[primitive.material], baseDir, path);
                        }

                        asset.m_parts.push_back(std::move(part));
                    }
                }

                if (asset.m_parts.empty())
                {
                    throw std::runtime_error("glTF '" + path + "' contains no valid triangle primitives");
                }

                return asset;
            }

            std::optional<TextureAsset> AssetUtils::LoadTexture(const std::string& path, TextureAsset::Type type)
            {
                Core::Log::Info("[Resource] Load texture: ", path);

                void* data = nullptr;
                int desiredChannels = GetTextureChannels(type);
                int texWidth = 0, texHeight = 0, texChannels = 0;
                if (type == TextureAsset::Type::Hdr)
                {
                    data = stbi_loadf(path.c_str(), &texWidth, &texHeight, &texChannels, desiredChannels);
                }
                else
                {
                    data = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, desiredChannels);
                }
                if (!data)
                {
                    Core::Log::Warning("[Resource] Failed to load texture: ", path);
                    return std::nullopt;
                }
                const size_t byteSize = static_cast<size_t>(texWidth)
                    * static_cast<size_t>(texHeight)
                    * GetBytesPerPixel(type);
                std::vector<uint8_t> bytes(
                    reinterpret_cast<const uint8_t*>(data),
                    reinterpret_cast<const uint8_t*>(data) + byteSize
                );
                stbi_image_free(data);

                TextureAsset tex{};
                tex.m_name = std::filesystem::path(path).stem().string();
                tex.m_bytes = std::move(bytes);
                tex.m_width = static_cast<uint32_t>(texWidth);
                tex.m_height = static_cast<uint32_t>(texHeight);
                tex.m_type = type;

                return tex;
            }
        }
    }
}
