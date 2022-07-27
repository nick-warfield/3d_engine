#include "frame_data.hpp"
#include "util.hpp"
#include "descriptor_builder.hpp"

#include <vulkan/vulkan_core.h>

namespace chch {

void FrameData::init(const Context* context)
{
	init_command_buffer(context);
	init_sync_objects(context);
}

void FrameData::deinit(const Context* context)
{
	vkDestroySemaphore(context->device, image_available_semaphore, context->allocation_callbacks);
	vkDestroySemaphore(context->device, render_finished_semaphore, context->allocation_callbacks);
	vkDestroyFence(context->device, in_flight_fence, context->allocation_callbacks);
	vkDestroyCommandPool(context->device, command_pool, context->allocation_callbacks);
}

void FrameData::init_command_buffer(const Context* context)
{
	VkCommandPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = context->graphics_queue.index;

	vk_check(vkCreateCommandPool(context->device, &pool_info, context->allocation_callbacks, &command_pool));

	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	vk_check(vkAllocateCommandBuffers(context->device, &alloc_info, &command_buffer));
}

void FrameData::init_sync_objects(const Context* context)
{
	VkSemaphoreCreateInfo semaphore_info {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	vk_check(vkCreateSemaphore(
			context->device,
			&semaphore_info,
			context->allocation_callbacks,
			&image_available_semaphore));

	vk_check(vkCreateSemaphore(
			context->device,
			&semaphore_info,
			context->allocation_callbacks,
			&render_finished_semaphore));

	vk_check(vkCreateFence(
			context->device,
			&fence_info,
			context->allocation_callbacks,
			&in_flight_fence));
}

}
