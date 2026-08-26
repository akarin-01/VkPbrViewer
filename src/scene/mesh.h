#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Scene
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

            uint64_t GetRevision() const { return m_revision; }

            bool IsEmpty() const;

            void SetData(const std::string& name, std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices);
            void SetEmpty();

        private:
            std::string m_name{ "empty" };
            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;
            uint64_t m_revision{ 0 };
        };
    }
}
