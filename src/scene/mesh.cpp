#include "mesh.h"

#include "core/log.h"
#include "scene/vertex.h"

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

        KITA_LOG_DEBUG("[Scene] Set mesh data: ", m_name, ", ",
            m_vertices.size(), " vertices, ",
            m_indices.size(), " indices");
    }

    void Mesh::SetEmpty()
    {
        SetData("empty", {}, {});
    }
}