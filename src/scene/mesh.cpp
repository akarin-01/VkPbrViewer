#include "mesh.h"

#include "core/log.h"
#include "scene/vertex.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <filesystem>

namespace Kita::Pbrv
{
    Mesh::Mesh() = default;

    Mesh::~Mesh() = default;

    const std::vector<Vertex>& Mesh::GetVertices() const
    {
        return m_vertices;
    }

    size_t Mesh::GetVertexCount() const
    {
        return m_vertices.size();
    }

    const std::vector<uint32_t>& Mesh::GetIndices() const
    {
        return m_indices;
    }

    size_t Mesh::GetIndexCount() const
    {
        return m_indices.size();
    }

    bool Mesh::IsEmpty() const
    {
        return m_vertices.empty() || m_indices.empty();
    }

    void Mesh::LoadFromObj(const std::string& path)
    {
        // Load mesh data
        Log::Info("[Scene] Load mesh: ", path);

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
            {
                throw std::runtime_error(warn + err);
            }

            std::unordered_map<Vertex, uint32_t> uniqueVertices{};
            for (const auto& shape : shapes)
            {
                for (const auto& index : shape.mesh.indices)
                {
                    Vertex vertex{};

                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };

                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                    };

                    if (uniqueVertices.count(vertex) == 0)
                    {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }

                    indices.push_back(uniqueVertices[vertex]);
                }
            }
        }

        std::string name = std::filesystem::path(path).stem().string();

        SetData(name,
            std::move(vertices), std::move(indices));
    }

    void Mesh::SetData(const std::string& name,
        std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
    {
        bool unchanged = m_name == name
            && m_vertices == vertices
            && m_indices == indices;
        if (unchanged)
        {
            KITA_LOG_DEBUG("[Scene] Set mesh data unchanged, skip: ", name);
            return;
        }

        m_name = name;
        m_vertices = std::move(vertices);
        m_indices = std::move(indices);
        m_isDirty = true;

        Log::Info("[Scene] Set mesh data: ", m_name, ", ",
            m_vertices.size(), " vertices, ",
            m_indices.size(), " indices");
    }

    void Mesh::SetEmpty()
    {
        SetData("empty", {}, {});
    }
}