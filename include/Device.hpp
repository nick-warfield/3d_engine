#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <optional>
#include <limits>

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamily {
	uint32_t index = std::numeric_limits<uint32_t>::max();
	VkQueue queue = VK_NULL_HANDLE;
};

struct Device {
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice logical_device = VK_NULL_HANDLE;

	QueueFamily graphics_queue_family;
	QueueFamily present_queue_family;

	void init();

	void deinit();

	bool supports_required_extensions()
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(
			physical_device,
			nullptr,
			&extension_count,
			available_extensions.data());

		for (const auto& req_ext : device_extensions) {
			bool extension_found = false;
			for (const auto& e : available_extensions) {
				extension_found = extension_found || (strcmp(e.extensionName, req_ext) == 0);
			}
			if (!extension_found) {
				return false;
			}
		}
		return true;
	}

	int score(const VkSurfaceKHR& surface)
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(physical_device, &device_properties);

		VkPhysicalDeviceFeatures dev_features;
		vkGetPhysicalDeviceFeatures(physical_device, &dev_features);

		if (queue_families.empty()) {
			queue_families = find_queue_families(surface, physical_device);
		}
		if (!supported_swap_chain_features.has_value() && supports_required_extensions()) {
			supported_swap_chain_features = querySwapChainSupport(physical_device, surface);
		}

		int score = 1000 * (int)(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
		score += device_properties.limits.maxImageDimension2D;

		bool has_required_features = queue_families[Device::GRAPHICS].index.has_value()
			&& queue_families[Device::PRESENT].index.has_value()
			&& !supported_swap_chain_features.value().formats.empty()
			&& !supported_swap_chain_features.value().present_modes.empty()
			&& dev_features.geometryShader;

		return score * (int)has_required_features;
	}
};

