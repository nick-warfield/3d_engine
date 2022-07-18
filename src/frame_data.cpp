#include "frame_data.hpp"
#include "device.hpp"

#include <vulkan/vulkan_core.h>
#include <stdexcept>

namespace gfx {

void FrameData::init(const Device& device)
{
	init_command_buffer(device);
	init_sync_objects(device);
}

void FrameData::deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator)
{
	vkDestroySemaphore(device, image_available_semaphore, pAllocator);
	vkDestroySemaphore(device, render_finished_semaphore, pAllocator);
	vkDestroyFence(device, in_flight_fence, pAllocator);
	vkDestroyCommandPool(device, command_pool, pAllocator);
}

void FrameData::init_command_buffer(const Device& device)
{
	auto dev = device.logical_device;
	VkCommandPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = device.graphics_queue_family.index.value();

	if (vkCreateCommandPool(dev, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateCommandBuffers(dev, &alloc_info, &command_buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffer");
}

void FrameData::init_sync_objects(const Device& device)
{
	VkSemaphoreCreateInfo semaphore_info {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	auto dev = device.logical_device;
	bool success = true;
	success = success
		&& vkCreateSemaphore(
				dev,
				&semaphore_info,
				nullptr,
				&image_available_semaphore)
		== VK_SUCCESS;

	success = success
		&& vkCreateSemaphore(
				dev,
				&semaphore_info,
				nullptr,
				&render_finished_semaphore)
		== VK_SUCCESS;

	success = success
		&& vkCreateFence(
				dev,
				&fence_info,
				nullptr,
				&in_flight_fence)
		== VK_SUCCESS;

	if (!success)
		throw std::runtime_error("failed to create synchronization objects");
}

}
