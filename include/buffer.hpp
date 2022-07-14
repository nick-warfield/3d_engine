#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

namespace gfx {

struct Device;

struct Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;

	void init(const Device& device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);
	void deinit(const Device& device, const VkAllocationCallbacks* = nullptr);
};

struct BufferData {
	Buffer index_buffer;
	Buffer vertex_buffer;
	std::vector<Buffer> uniform_buffers;

	Buffer staging_buffer;
	VkCommandPool copy_command_pool;

	void init(const Device& device);
	void deinit(const Device& device, const VkAllocationCallbacks* = nullptr);
	void update_uniform_buffer(
		const VkDevice& device,
		VkExtent2D extent,
		uint32_t current_image);

private:
	void copy_buffer(const Device& device,
		VkBuffer src_buffer,
		VkBuffer dst_buffer,
		VkDeviceSize size);
};

}
