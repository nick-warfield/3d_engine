#include "buffer.hpp"
#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <stdexcept>

namespace gfx {

void Buffer::init(const Device& device,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo buffer_info {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;

	auto queue_family_indices = device.get_unique_queue_family_indices();
	if (queue_family_indices.size() > 1) {
		buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_info.queueFamilyIndexCount = queue_family_indices.size();
		buffer_info.pQueueFamilyIndices = queue_family_indices.data();
	} else {
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_info.queueFamilyIndexCount = 0;
		buffer_info.pQueueFamilyIndices = nullptr;
	}

	auto dev = device.logical_device;
	if (vkCreateBuffer(dev, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer");

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(dev, buffer, &memory_requirements);

	auto mem_index = device.find_memory_type(
		memory_requirements.memoryTypeBits,
		properties);

	VkMemoryAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = mem_index;

	if (vkAllocateMemory(dev, &alloc_info, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory");

	vkBindBufferMemory(dev, buffer, memory, 0);
}

void Buffer::deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator)
{
	vkDestroyBuffer(device, buffer, pAllocator);
	vkFreeMemory(device, memory, pAllocator);
}

}
