#include "buffer.hpp"
#include "context.hpp"
#include "vk_mem_alloc.h"
#include "util.hpp"

#include <vulkan/vulkan_core.h>

namespace chch {

void Buffer::init(const Context* context,
	VkDeviceSize size,
	VkBufferUsageFlags buffer_usage,
	VkMemoryPropertyFlags memory_properties,
	VmaMemoryUsage memory_usage)
{
	VkBufferCreateInfo buffer_info {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = buffer_usage;

	if (context->unique_queue_indices.size() > 1) {
		buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_info.queueFamilyIndexCount = context->unique_queue_indices.size();
		buffer_info.pQueueFamilyIndices = context->unique_queue_indices.data();
	} else {
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_info.queueFamilyIndexCount = 0;
		buffer_info.pQueueFamilyIndices = nullptr;
	}

	VmaAllocationCreateInfo alloc_info {};
	alloc_info.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
	alloc_info.usage = memory_usage;
	alloc_info.requiredFlags = memory_properties;
	alloc_info.memoryTypeBits = 0;

	auto result = vmaCreateBuffer(context->allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
	vk_check(result);
}

void Buffer::deinit(const Context* context) {
	vmaDestroyBuffer(context->allocator, buffer, allocation);
}

}
