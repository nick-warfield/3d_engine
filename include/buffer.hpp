#pragma once

#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"

namespace chch {

struct Context;

struct Buffer {
	VkBuffer buffer;
	VmaAllocation allocation;

	void init(const Context* context,
	VkDeviceSize size,
	VkBufferUsageFlags buffer_usage,
	VkMemoryPropertyFlags memory_properties,
	VmaMemoryUsage memory_usage);
	void deinit(const Context* context);
};

}
