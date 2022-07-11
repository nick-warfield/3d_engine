#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include <optional>

struct SystemInfo {
	std::optional<VkSurfaceCapabilitiesKHR> surface_capabilities;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;


	void update_swap_chain_support(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
	{
		VkSurfaceCapabilitiesKHR temp_cap;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &temp_cap);
		surface_capabilities = temp_cap;

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
		if (format_count != 0) {
			surface_formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(
				device,
				surface,
				&format_count,
				surface_formats.data());
		}

		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
		if (present_mode_count != 0) {
			present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				device,
				surface,
				&present_mode_count,
				present_modes.data());
		}
	}

	void update_queue_family_indices(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
	{
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

		for (uint32_t i = 0; i < queue_family_count; ++i) {
			if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
				graphics_queue_family.index = i;

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
			if (present_support)
				present_queue_family.index = i;
		}
	}

	void update_queue_family_queues(const VkDevice& device) {
		if (!graphics_queue_family.index.has_value()
				|| !present_queue_family.index.has_value())
			return;

		VkQueue temp_queue;
		vkGetDeviceQueue(
			device,
			graphics_queue_family.index.value(),
			0,
			&temp_queue);
		graphics_queue_family.queue = temp_queue;

		vkGetDeviceQueue(
			device,
			present_queue_family.index.value(),
			0,
			&temp_queue);
		present_queue_family.queue = temp_queue;
	}
};
