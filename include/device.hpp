#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace gfx {

struct Window;

struct QueueFamily {
	std::optional<uint32_t> index;
	VkQueue queue = VK_NULL_HANDLE;
};

struct SwapChainSupportInfo {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

struct Device {
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice logical_device = VK_NULL_HANDLE;

	VkPhysicalDeviceMemoryProperties memory_properties;
	QueueFamily graphics_queue_family;
	QueueFamily present_queue_family;
	SwapChainSupportInfo supported_swap_chain_features;

	void init(const VkInstance& instance, const VkSurfaceKHR& surface);
	void deinit(const VkAllocationCallbacks* pAllocator = nullptr);

	void update_swap_chain_support_info(const Window& window);

private:
	bool supports_required_extensions();
	void populate_swap_chain_support_info(const VkSurfaceKHR& surface);
	void populate_queue_family_indices(const VkSurfaceKHR& surface);
	int score_device();
	void init_logical_device();
};

}
