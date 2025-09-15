#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
    Surface::Surface(const Instance& instance, const HWND wndHandle) : m_Instance(instance) {
        m_WndHandle = wndHandle;

        VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        win32SurfaceCreateInfo.hwnd = wndHandle;
        win32SurfaceCreateInfo.hinstance = GetModuleHandleA(nullptr); // This might be bad?

        if (const auto result = vkCreateWin32SurfaceKHR(instance, &win32SurfaceCreateInfo, nullptr, &m_Surface); result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create win32 surface: " + std::to_string(result));
        }
    }

    Surface::~Surface() {
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    }

    QueueFamilyIndices_t Surface::FindQueueFamilyIndices(std::shared_ptr<PhysicalDevice> physicalDevice) const {
        QueueFamilyIndices_t out{};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(*physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(*physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (auto i{ 0u }; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                out.m_GraphicsFamilyIndex = i;

            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                out.m_ComputeFamilyIndex = i;

            VkBool32 presentSupported{ false };
            vkGetPhysicalDeviceSurfaceSupportKHR(*physicalDevice, i, m_Surface, &presentSupported);
            if (presentSupported)
                out.m_PresentFamilyIndex = i;

            if (out.IsValid())
                break;
        }

        return out;
    }

    SwaphchainSupportData_t Surface::QuerySwapchainSupport(std::shared_ptr<PhysicalDevice> physicalDevice) const {
        SwaphchainSupportData_t out{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*physicalDevice, m_Surface, &out.m_SurfaceCapabilities);
        
        uint32_t formatCount{};
        vkGetPhysicalDeviceSurfaceFormatsKHR(*physicalDevice, m_Surface, &formatCount, nullptr);

        if (formatCount != 0) {
            out.m_SurfaceFormats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(*physicalDevice, m_Surface, &formatCount, out.m_SurfaceFormats.data());
        }

        uint32_t presentModeCount{};
        vkGetPhysicalDeviceSurfacePresentModesKHR(*physicalDevice, m_Surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            out.m_PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(*physicalDevice, m_Surface, &presentModeCount, out.m_PresentModes.data());
        }

        return out;
    }
}