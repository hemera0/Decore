#pragma once

namespace Engine::Renderer {
	struct QueueFamilyIndices_t {
		uint32_t m_GraphicsFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		uint32_t m_PresentFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		uint32_t m_ComputeFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		bool IsValid() const {
			return m_GraphicsFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
				m_PresentFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
				m_ComputeFamilyIndex != VK_QUEUE_FAMILY_IGNORED;
		}
	};

	struct SwaphchainSupportData_t {
		VkSurfaceCapabilitiesKHR m_SurfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> m_SurfaceFormats{};
		std::vector<VkPresentModeKHR> m_PresentModes{};
	};

	class Surface {
	private:
		HWND m_WndHandle{};
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		const Instance& m_Instance;
	public:
		Surface(const Instance& instance, HWND wndHandle);
		~Surface();

		QueueFamilyIndices_t FindQueueFamilyIndices(std::shared_ptr<PhysicalDevice> physicalDevice) const;
		SwaphchainSupportData_t QuerySwapchainSupport(std::shared_ptr<PhysicalDevice> physicalDevice) const;

		HWND GetWindowHandle() const { return m_WndHandle; }

		DEFINE_IMPLICIT_VK(m_Surface);
	};
}