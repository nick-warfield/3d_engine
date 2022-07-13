#pragma once

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
	Buffer staging_buffer;
	VkCommandPool copy_command_pool;

	void init(const Device& device);
	void deinit(const Device& device, const VkAllocationCallbacks* = nullptr);

private:
	void copy_buffer(const Device& device,
			VkBuffer src_buffer,
			VkBuffer dst_buffer,
			VkDeviceSize size);
};

}
