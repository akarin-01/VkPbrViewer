#include "context.h"

#include "core/log.h"
#include "core/window.h"
#include "rhi/utils.h"

#include <vector>
#include <stdexcept>
#include <optional>
#include <set>
#include <string>
#include <cstring>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        namespace
        {
    #ifdef NDEBUG
            const bool enableValidationLayers = false;
    #else
            const bool enableValidationLayers = true;
    #endif

            const std::vector<const char*> validationLayers =
            {
                "VK_LAYER_KHRONOS_validation"
            };

            const std::vector<const char*> deviceExtensions =
            {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            bool CheckValidationLayerSupport()
            {
                uint32_t layerCount;
                vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

                std::vector<VkLayerProperties> availableLayers(layerCount);
                vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

                for (const char* layerName : validationLayers)
                {
                    bool layerFound = false;
                    for (const auto& layerProperties : availableLayers)
                    {
                        if (strcmp(layerName, layerProperties.layerName) == 0)
                        {
                            layerFound = true;
                            break;
                        }
                    }

                    if (!layerFound)
                    {
                        return false;
                    }
                }

                return true;
            }

            VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                VkDebugUtilsMessageTypeFlagsEXT /*type*/,
                const VkDebugUtilsMessengerCallbackDataEXT* data,
                void* /*userData*/)
            {
                switch (severity)
                {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    Core::Log::Error("[Vulkan] ", data->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    Core::Log::Warning("[Vulkan] ", data->pMessage);
                    break;
                default:   // INFO / VERBOSE
                    Core::Log::Info("[Vulkan] ", data->pMessage);
                    break;
                }
                return VK_FALSE;   // don't abort
            }

            void FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
            {
                createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                createInfo.messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                createInfo.messageType =
                    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                createInfo.pfnUserCallback = DebugCallback;
            }

            VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* createInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* debugMessenger)
            {
                auto createFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
                if (!createFunc)
                {
                    return VK_ERROR_EXTENSION_NOT_PRESENT;
                }

                return createFunc(instance, createInfo, pAllocator, debugMessenger);
            }

            void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
            {
                auto destroyFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
                if (destroyFunc)
                {
                    destroyFunc(instance, debugMessenger, pAllocator);
                }
            }

            bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
            {
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

                std::vector<VkExtensionProperties> availableExtensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

                std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

                for (const auto& extension : availableExtensions)
                {
                    requiredExtensions.erase(extension.extensionName);
                }

                return requiredExtensions.empty();
            }

            bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
            {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);

                if (deviceProperties.apiVersion < VK_API_VERSION_1_3)
                {
                    Core::Log::Info("[Device Check] Skip ", deviceProperties.deviceName, ": API version too low.");
                    return false;
                }

                QueueFamilyIndices indices = FindQueueFamilies(device, surface);

                bool extensionsSupported = CheckDeviceExtensionSupport(device);

                bool swapChainAdequate = false;
                if (extensionsSupported)
                {
                    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
                    swapChainAdequate = !swapChainSupport.m_formats.empty()
                        && !swapChainSupport.m_presentModes.empty();
                }

                VkPhysicalDeviceVulkan13Features queryVulkan13Features{};
                queryVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
                VkPhysicalDeviceFeatures2 queryDeviceFeatures2{};
                queryDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                queryDeviceFeatures2.pNext = &queryVulkan13Features;
                vkGetPhysicalDeviceFeatures2(device, &queryDeviceFeatures2);
                bool featuresSupported = queryDeviceFeatures2.features.samplerAnisotropy
                    && queryVulkan13Features.dynamicRendering && queryVulkan13Features.synchronization2;

                return indices.IsComplete() && extensionsSupported && swapChainAdequate
                    && featuresSupported;
            }

            VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
            {
                for (const auto& format : candidates)
                {
                    VkFormatProperties properties{};
                    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

                    if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
                    {
                        return format;
                    }
                    else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
                    {
                        return format;
                    }
                }

                throw std::runtime_error("Failed to find supported format!");
            }

            VkFormat PickDepthFormat(VkPhysicalDevice physicalDevice)
            {
                return FindSupportedFormat(physicalDevice,
                    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
            }

            float PickMaxAnisotropy(VkPhysicalDeviceProperties properties)
            {
                return properties.limits.maxSamplerAnisotropy;
            }

            VkSampleCountFlags PickSupportedSampleCounts(VkPhysicalDeviceProperties properties)
            {
                return properties.limits.framebufferColorSampleCounts &
                    properties.limits.framebufferDepthSampleCounts;
            }

            VkSampleCountFlagBits PickMaxSampleCount(VkSampleCountFlags supported)
            {
                if (supported & VK_SAMPLE_COUNT_64_BIT)
                {
                    return VK_SAMPLE_COUNT_64_BIT;
                }
                if (supported & VK_SAMPLE_COUNT_32_BIT)
                {
                    return VK_SAMPLE_COUNT_32_BIT;
                }
                if (supported & VK_SAMPLE_COUNT_16_BIT)
                {
                    return VK_SAMPLE_COUNT_16_BIT;
                }
                if (supported & VK_SAMPLE_COUNT_8_BIT)
                {
                    return VK_SAMPLE_COUNT_8_BIT;
                }
                if (supported & VK_SAMPLE_COUNT_4_BIT)
                {
                    return VK_SAMPLE_COUNT_4_BIT;
                }
                if (supported & VK_SAMPLE_COUNT_2_BIT)
                {
                    return VK_SAMPLE_COUNT_2_BIT;
                }
                return VK_SAMPLE_COUNT_1_BIT;
            }

            VkSampleCountFlagBits PickSampleCount(VkSampleCountFlags supported)
            {
                // 4x MSAA: balanced quality/performance; fall back to 1x if unsupported
                return (supported & VK_SAMPLE_COUNT_4_BIT) ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;
            }
        }

        RenderContext::RenderContext(const Core::Window& window)
            : m_window(window)
        {
            CreateInstance();
            SetupDebugMessenger();
            CreateSurface();
            PickPhysicalDevice();
            CreateLogicalDevice();
            CreateCommandPool();
        }

        RenderContext::~RenderContext()
        {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            vkDestroyDevice(m_device, nullptr);
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
            vkDestroyInstance(m_instance, nullptr);
        }

        void RenderContext::CreateInstance()
        {
            if (enableValidationLayers && !CheckValidationLayerSupport())
            {
                throw std::runtime_error("Validation layers requested, but not available!");
            }

            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Vk Pbr Viewer";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Kita Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_3;

            auto extensions = m_window.GetRequiredInstanceExtensions();
            if (enableValidationLayers)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            if (enableValidationLayers)
            {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();

                VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
                FillDebugMessengerCreateInfo(debugCreateInfo);
                createInfo.pNext = &debugCreateInfo;
            }
            else
            {
                createInfo.enabledLayerCount = 0;
            }

            if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create instance!");
            }
        }

        void RenderContext::SetupDebugMessenger()
        {
            if (!enableValidationLayers)
            {
                return;
            }

            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            FillDebugMessengerCreateInfo(createInfo);

            if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to set up debug messenger!");
            }
        }

        void RenderContext::CreateSurface()
        {
            m_surface = m_window.CreateSurface(m_instance);
        }

        void RenderContext::PickPhysicalDevice()
        {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

            if (deviceCount == 0)
            {
                throw std::runtime_error("Failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

            for (const auto& device : devices)
            {
                if (IsDeviceSuitable(device, m_surface))
                {
                    m_physicalDevice = device;
                    break;
                }
            }

            if (m_physicalDevice == VK_NULL_HANDLE)
            {
                throw std::runtime_error("Failed to find a suitable GPU!");
            }

            CachePhysicalDeviceCaps();
        }

        void RenderContext::CreateLogicalDevice()
        {
            QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);

            std::set<uint32_t> uniqueQueueFamilies = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value() };
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

            float queuePriority = 1.0f;
            for (const auto& uniqueQueueFamily : uniqueQueueFamilies)
            {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = uniqueQueueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;

                queueCreateInfos.push_back(queueCreateInfo);
            }

            // Features to enable
            VkPhysicalDeviceVulkan13Features enabledVulkan13Features{};
            enabledVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            enabledVulkan13Features.dynamicRendering = VK_TRUE;
            enabledVulkan13Features.synchronization2 = VK_TRUE;
            VkPhysicalDeviceFeatures2 enabledDeviceFeatures2{};
            enabledDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            enabledDeviceFeatures2.pNext = &enabledVulkan13Features;
            enabledDeviceFeatures2.features.samplerAnisotropy = VK_TRUE;

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.pNext = &enabledDeviceFeatures2;
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();

            if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create logical device!");
            }

            vkGetDeviceQueue(m_device, indices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
            vkGetDeviceQueue(m_device, indices.m_presentFamily.value(), 0, &m_presentQueue);

            m_graphicsFamily = indices.m_graphicsFamily.value();
            m_presentFamily = indices.m_presentFamily.value();
        }

        void RenderContext::CreateCommandPool()
        {
            QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice, m_surface);

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();

            if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create command pool!");
            }
        }

        void RenderContext::CachePhysicalDeviceCaps()
        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

            m_depthFormat = PickDepthFormat(m_physicalDevice);
            m_hdrFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
            m_maxAnisotropy = PickMaxAnisotropy(properties);
            m_supportedSampleCounts = PickSupportedSampleCounts(properties);
            m_maxSampleCount = PickMaxSampleCount(m_supportedSampleCounts);
            m_sampleCount = PickSampleCount(m_supportedSampleCounts);
        }
    }
}
