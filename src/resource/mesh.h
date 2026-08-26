#pragma once

#include "resource/handle.h"
#include "resource/vertex.h"

#include <string>
#include <vector>

namespace Kita::Pbrv
{
    namespace Resource
    {
        struct Mesh
        {
            using Handle = Resource::Handle<Mesh>;   // asset handle

            std::string m_name{ "empty" };
            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;

            size_t GetVertexCount() const { return m_vertices.size(); }
            size_t GetIndexCount() const { return m_indices.size(); }
        };
    }
}
