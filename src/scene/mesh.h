#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace Kita::Pbrv
{
    struct Vertex;

    class Mesh
    {
    public:
        Mesh();
        ~Mesh();

        const std::string& GetName() const { return m_name; }
        const std::vector<Vertex>& GetVertices() const;
        size_t GetVertexCount() const;
        const std::vector<uint32_t>& GetIndices() const;
        size_t GetIndexCount() const;

        bool IsDirty() const { return m_isDirty; }
        void ClearDirty() const { m_isDirty = false; }

        bool IsEmpty() const;

        void LoadFromObj(const std::string& path);
        void SetData(const std::string& name, std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices);

    private:
        std::string m_name;
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        mutable bool m_isDirty{ false };
    };
}