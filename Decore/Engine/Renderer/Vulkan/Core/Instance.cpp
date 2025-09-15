#include "../VulkanRenderer.hpp"

constexpr bool UseValidationLayers = true;

namespace Engine::Renderer {
	Instance::Instance(const char** requiredExtensions, uint32_t requiredExtensionsCount) {
		VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.pEngineName = "Decore";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_4;

		VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.ppEnabledLayerNames = nullptr;

		if (requiredExtensions && requiredExtensionsCount) {
			instanceCreateInfo.enabledExtensionCount = requiredExtensionsCount;
			instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions;
		}

		if constexpr (UseValidationLayers) {
			std::array<const char*, 1> validationLayers = {
				"VK_LAYER_KHRONOS_validation",
			};

			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			size_t supportedLayersCount = 0;

			for (const auto& wantedLayer : validationLayers) {
				for (const auto& availableLayer : availableLayers) {
					if (strcmp(wantedLayer, availableLayer.layerName) == 0)
						supportedLayersCount++;
				}
			}

			// disable for NSight
			VkValidationFeaturesEXT extValidationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };

			auto disableUniqueHandles = VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT;

			extValidationFeatures.disabledValidationFeatureCount = 1;
			extValidationFeatures.pDisabledValidationFeatures = &disableUniqueHandles;

			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
			instanceCreateInfo.pNext = &extValidationFeatures;
		}

		VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

		printf("Created VkInstance: 0x%p\n", instance);
	}

	Instance::~Instance() {
		vkDestroyInstance(instance, nullptr);
	}
}