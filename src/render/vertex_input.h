#pragma once

#include "resource/vertex.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Kita::Pbrv
{
    namespace Render
    {
        /// Pipeline vertex-input description for Resource::Vertex; locations
        /// match the current vertex shader interface.
        namespace VertexInput
        {
            inline VkVertexInputBindingDescription Binding()
            {
                VkVertexInputBindingDescription binding{};
                binding.binding = 0;
                binding.stride = sizeof(Resource::Vertex);
                binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return binding;
            }

            inline std::vector<VkVertexInputAttributeDescription> Attributes()
            {
                std::vector<VkVertexInputAttributeDescription> attributes(4);

                attributes[0].binding = 0;
                attributes[0].location = 0;
                attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributes[0].offset = offsetof(Resource::Vertex, position);

                attributes[1].binding = 0;
                attributes[1].location = 1;
                attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributes[1].offset = offsetof(Resource::Vertex, normal);

                attributes[2].binding = 0;
                attributes[2].location = 2;
                attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
                attributes[2].offset = offsetof(Resource::Vertex, texCoord);

                attributes[3].binding = 0;
                attributes[3].location = 3;
                attributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                attributes[3].offset = offsetof(Resource::Vertex, tangent);

                return attributes;
            }
        }
    }
}
