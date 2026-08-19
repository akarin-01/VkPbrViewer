#pragma once

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec4 tangent;      // w = handedness

        bool operator==(const Vertex& other) const
        {
            return position == other.position
                && normal == other.normal
                && texCoord == other.texCoord
                && tangent == other.tangent;
        }
    };
}