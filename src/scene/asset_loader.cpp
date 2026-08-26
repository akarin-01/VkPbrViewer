#include "asset_loader.h"
#include "core/log.h"
#include "scene/vertex.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <cmath>

namespace Kita::Pbrv
{
    namespace Scene
    {
        namespace
        {
            int GetTextureChannels(TextureType type)
            {
                switch (type)
                {
                case TextureType::Srgb:                 return 4;
                case TextureType::Normal:               return 4;
                case TextureType::MetallicRoughness:    return 4;
                case TextureType::Linear:               return 1;
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
                    Core::Log::Warning("[Scene] Index count is not a multiple of 3, skipping tangent computation");
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
        }

        void AssetLoader::LoadGltfMesh(const std::string& path, Mesh& mesh)
        {
            Core::Log::Info("[Scene] Load glTF: ", path);

            tinygltf::Model model;
            tinygltf::TinyGLTF loader;
            std::string err;
            std::string warn;

            bool success = (std::filesystem::path(path).extension() == ".glb")
                ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
                : loader.LoadASCIIFromFile(&model, &err, &warn, path);

            if (!warn.empty())
            {
                Core::Log::Warning("[Scene] glTF warning: ", warn);
            }
            if (!success)
            {
                throw std::runtime_error("Failed to load glTF: " + err);
            }
            if (model.meshes.empty())
            {
                throw std::runtime_error("glTF contains no meshes");
            }

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            // Only support 1 mesh
            for (const auto& primitive : model.meshes[0].primitives)
            {
                if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                {
                    Core::Log::Warning("[Scene] Unsupported primitive mode, skipped");
                    continue;
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

                const uint32_t baseVertexIdx = static_cast<uint32_t>(vertices.size());
                vertices.resize(vertices.size() + positionAccessor.count);
                for (size_t i = 0; i < positionAccessor.count; ++i)
                {
                    Vertex& vertex = vertices[baseVertexIdx + i];
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
                    indices.reserve(indices.size() + positionAccessor.count);
                    for (size_t i = 0; i < positionAccessor.count; ++i)
                    {
                        indices.emplace_back(baseVertexIdx + static_cast<uint32_t>(i));
                    }
                }
                else
                {
                    const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                    indices.reserve(indices.size() + indexAccessor.count);
                    for (size_t i = 0; i < indexAccessor.count; ++i)
                    {
                        indices.emplace_back(baseVertexIdx + ReadAccessorIndex(model, indexAccessor, i));
                    }
                }
            }

            ComputeTangents(vertices, indices);

            std::string name = std::filesystem::path(path).stem().string();

            mesh.SetData(name, std::move(vertices), std::move(indices));
        }

        void AssetLoader::LoadTexture(const std::string& path, TextureType type, Texture& texture)
        {
            Core::Log::Info("[Scene] Load texture: ", path);

            std::vector<uint8_t> pixels;
            uint32_t width, height;
            {
                int desiredChannels = GetTextureChannels(type);

                int texWidth, texHeight, texChannels;
                stbi_uc* data = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, desiredChannels);
                if (!data)
                {
                    throw std::runtime_error("Failed to load texture image");
                }
                size_t byteSize = static_cast<size_t>(texWidth)
                    * static_cast<size_t>(texHeight)
                    * static_cast<size_t>(desiredChannels);
                pixels.assign(data, data + byteSize);
                width = static_cast<uint32_t>(texWidth);
                height = static_cast<uint32_t>(texHeight);

                stbi_image_free(data);
            }

            std::string name = std::filesystem::path(path).stem().string();

            texture.SetData(name, std::move(pixels), width, height, type);
        }

        void AssetLoader::LoadSkybox(const std::string& path, Skybox& skybox)
        {
            Core::Log::Info("[Scene] Load skybox: ", path);

            std::vector<float> pixels;
            uint32_t width, height;
            {
                int desiredChannels = 4;
                int texWidth, texHeight, texChannels;
                float* data = stbi_loadf(path.c_str(), &texWidth, &texHeight, &texChannels, desiredChannels);
                if (!data)
                {
                    throw std::runtime_error("Failed to load skybox image");
                }
                size_t byteSize = static_cast<size_t>(texWidth)
                    * static_cast<size_t>(texHeight)
                    * static_cast<size_t>(desiredChannels);
                pixels.assign(data, data + byteSize);
                width = static_cast<uint32_t>(texWidth);
                height = static_cast<uint32_t>(texHeight);

                stbi_image_free(data);
            }

            std::string name = std::filesystem::path(path).stem().string();

            skybox.SetData(name, std::move(pixels), width, height);
        }
    }
}
