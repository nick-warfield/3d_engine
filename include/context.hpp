#pragma once

#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include <vector>

namespace gfx {

struct Context {
	const std::vector<const char*> validation_layers = {
		"VK_LAYER_KHRONOS_validation",
	};

	const std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
		VK_AMD_MEMORY_OVERALLOCATION_BEHAVIOR_EXTENSION_NAME
	};

	// handles
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice physical_device;
	VkDevice logical_device;
	VmaAllocator allocator;

	// supported properties & features
};

}
