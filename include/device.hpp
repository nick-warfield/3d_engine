#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>
#include <functional>

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

	SwapChainSupportInfo supported_swap_chain_features;
	VkPhysicalDeviceMemoryProperties memory_properties;

	QueueFamily graphics_queue_family;
	QueueFamily present_queue_family;
	QueueFamily transfer_queue_family;

	void init(const VkInstance& instance, const VkSurfaceKHR& surface);
	void deinit(const VkAllocationCallbacks* pAllocator = nullptr);

	void update_swap_chain_support_info(const Window& window);
	std::vector<uint32_t> get_unique_queue_family_indices() const;
	uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags property_flags) const;
	void record_graphics_commands(std::function<void(VkCommandBuffer)> commands) const;
	void record_transfer_commands(std::function<void(VkCommandBuffer)> commands) const;

private:
	bool supports_required_extensions();
	void populate_swap_chain_support_info(const VkSurfaceKHR& surface);
	void populate_queue_family_indices(const VkSurfaceKHR& surface);
	int score_device();
	void init_logical_device();

	void record_commands(
			VkCommandPool command_pool,
			VkQueue queue,
			std::function<void(VkCommandBuffer command_buffer)> commands) const;
	VkCommandPool temp_command_pool;
	VkCommandPool graphics_command_pool;
};

}
