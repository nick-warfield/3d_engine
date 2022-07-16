#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

namespace gfx {

struct Device;
struct Vertex;

struct Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;

	void init(const Device& device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);
	void deinit(const Device& device, const VkAllocationCallbacks* = nullptr);
};

}
