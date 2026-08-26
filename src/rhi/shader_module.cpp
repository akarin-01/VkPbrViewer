#include "shader_module.h"

#include <stdexcept>
#include <vector>
#include <fstream>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        namespace
        {
            std::vector<char> ReadFile(const std::string& path)
            {
                std::ifstream file(path, std::ios::ate | std::ios::binary);

                if (!file.is_open())
                {
                    throw std::runtime_error("Failed to open file " + path + "!");
                }

                size_t filesize = static_cast<size_t>(file.tellg());
                std::vector<char> buffer(filesize);

                file.seekg(0);
                file.read(buffer.data(), filesize);

                file.close();

                return buffer;
            }
        }

        ShaderModule::ShaderModule(VkDevice device, const std::string& filePath)
            : m_device(device)
        {
            auto code = ReadFile(filePath);

            VkShaderModuleCreateInfo shaderInfo{};
            shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderInfo.codeSize = code.size();
            shaderInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            if (vkCreateShaderModule(m_device, &shaderInfo, nullptr, &m_shaderModule) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create shader module!");
            }
        }

        ShaderModule::~ShaderModule()
        {
            vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
        }
    }
}
