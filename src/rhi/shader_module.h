#pragma once

#include <vulkan/vulkan.h>
#include <string>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class ShaderModule
        {
        public:
            ShaderModule(VkDevice device, const std::string& filePath);
            ~ShaderModule();

            ShaderModule(const ShaderModule&) = delete;
            ShaderModule& operator=(const ShaderModule&) = delete;

            VkShaderModule Handle() const { return m_shaderModule; }

        private:
            VkDevice m_device{ VK_NULL_HANDLE };
            VkShaderModule m_shaderModule{ VK_NULL_HANDLE };
        };
    }
}
