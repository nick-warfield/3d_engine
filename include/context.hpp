#pragma once

#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <vector> // small vector would be nice here
#include <limits>

namespace chch {

struct ContextCreateInfo {
	const char* app_name;
	uint32_t app_version;
	VkExtent2D window_size;
	bool enable_validation_layers;
	std::vector<const char*> validation_layers;
	std::vector<const char*> required_extensions;
	std::vector<const char*> preferred_extensions;
	VkAllocationCallbacks* allocation_callbacks = nullptr;
};

// TODO: wrap window api
// TODO: add logger
struct Context {
	// handles
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VmaAllocator allocator;
	VkSurfaceKHR surface;
	GLFWwindow* window;
	VkAllocationCallbacks* allocation_callbacks;

	struct QueueFamily {
		uint32_t index = std::numeric_limits<uint32_t>::max();
		VkQueue queue = VK_NULL_HANDLE;

		bool is_available() {
			return index != std::numeric_limits<uint32_t>::max();
		}
	};
	QueueFamily graphics_queue;
	QueueFamily transfer_queue;
	QueueFamily present_queue;
	std::vector<uint32_t> unique_queue_indices;

	// supported properties & features
	VkSurfaceCapabilitiesKHR surface_capabilities;
	std::vector<VkSurfaceFormatKHR> supported_surface_formats;
	VkSurfaceFormatKHR surface_format;
	std::vector<VkPresentModeKHR> supported_present_modes;
	VkPresentModeKHR present_mode;

	VkPhysicalDeviceMemoryProperties memory_properties;
	VkPhysicalDeviceProperties device_properties;
	// right now i'm enable all available features, which is probably bad
	VkPhysicalDeviceFeatures device_features;
	VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT;

	bool window_resized = false;
	VkExtent2D window_size;

	bool enable_validation_layers;
	std::vector<const char*> validation_layers;
	std::vector<const char*> required_extensions;
	std::vector<const char*> preferred_extensions;
	std::vector<const char*> found_extensions;

	void init(const ContextCreateInfo& create_info);
	void deinit();

	// helpers
	uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags property_flags) const;
	bool window_hidden();

private:
	bool supports_required_extensions();
	void populate_surface_info();
	void populate_queue_family_indices();
	void set_max_usable_sample_count();
	void populate_all_info();
	int score_device();

	void init_glfw(const ContextCreateInfo& create_info);
	void init_instance(const ContextCreateInfo& create_info);
	void init_debugger();
	void init_surface();
	void init_physical_device();
	void init_logical_device();
	void init_queues();
	void init_allocator();
};

VkDebugUtilsMessengerCreateInfoEXT make_debugger_create_info();

}

