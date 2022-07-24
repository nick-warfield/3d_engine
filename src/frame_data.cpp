#include "frame_data.hpp"
#include <vulkan/vulkan_core.h>
#include <stdexcept>

namespace chch {

VkResult FrameData::init(const Context* context)
{
	VK_CHECK(init_command_buffer(context));
	if (res != VK_SUCCESS) return res;
	res = init_sync_objects(context);

	return res;
}

void FrameData::deinit(const Context* context)
{
	vkDestroySemaphore(context->device, image_available_semaphore, context->allocation_callbacks);
	vkDestroySemaphore(context->device, render_finished_semaphore, context->allocation_callbacks);
	vkDestroyFence(context->device, in_flight_fence, context->allocation_callbacks);
	vkDestroyCommandPool(context->device, command_pool, context->allocation_callbacks);
}

VkResult FrameData::init_command_buffer(const Context* context)
{
	VkCommandPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = context->graphics_queue.index;

	if (vkCreateCommandPool(context->device, &pool_info, context->allocation_callbacks, &command_pool) != VK_SUCCESS)
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
