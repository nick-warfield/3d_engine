#include "buffer.hpp"
#include "constants.hpp"
#include "device.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/trigonometric.hpp"
#include "uniform_buffer_object.hpp"

#include <iterator>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <cstring>
#include <stdexcept>
#include <cstring>

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
	if (queue_family_indices.size() > 0) {
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

	auto find_memory_type = [device](
								uint32_t type_filter,
								VkMemoryPropertyFlags property_flags) {
		auto mem_props = device.memory_properties;
		for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
			auto flags = mem_props.memoryTypes[i].propertyFlags;
			if (type_filter & (1 << i)
				&& (flags & property_flags) == property_flags)
				return i;
		}

		throw std::runtime_error("failed to find suitable memory type");
	};

	auto mem_index = find_memory_type(
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

void Buffer::deinit(const Device& device, const VkAllocationCallbacks* pAllocator)
{
	vkDestroyBuffer(device.logical_device, buffer, pAllocator);
	vkFreeMemory(device.logical_device, memory, pAllocator);
}

void BufferData::init(const Device& device)
{
	VkCommandPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	pool_info.queueFamilyIndex = device.transfer_queue_family.index.value();
	if (vkCreateCommandPool(device.logical_device, &pool_info, nullptr, &copy_command_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	// Create Vertex Buffer
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
	staging_buffer.init(device,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(device.logical_device, staging_buffer.memory, 0, buffer_size, 0, &data);
	memcpy(data, vertices.data(), (size_t)buffer_size);
	vkUnmapMemory(device.logical_device, staging_buffer.memory);

	vertex_buffer.init(device,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	copy_buffer(device, staging_buffer.buffer, vertex_buffer.buffer, buffer_size);

	// Create Index Buffer
	buffer_size = sizeof(indices[0]) * indices.size();
	staging_buffer.deinit(device);
	staging_buffer.init(device,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkMapMemory(device.logical_device, staging_buffer.memory, 0, buffer_size, 0, &data);
	memcpy(data, indices.data(), (size_t)buffer_size);
	vkUnmapMemory(device.logical_device, staging_buffer.memory);

	index_buffer.init(device,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	copy_buffer(device, staging_buffer.buffer, index_buffer.buffer, buffer_size);

	// Create UBO buffers
	buffer_size = sizeof(UniformBufferObject);
	uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (auto& ubo : uniform_buffers) {
		ubo.init(
			device,
			buffer_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
}

void BufferData::deinit(const Device& device, const VkAllocationCallbacks* pAllocator)
{
	index_buffer.deinit(device, pAllocator);
	vertex_buffer.deinit(device, pAllocator);
	for (auto ubo : uniform_buffers)
		ubo.deinit(device, pAllocator);

	staging_buffer.deinit(device, pAllocator);
	vkDestroyCommandPool(device.logical_device, copy_command_pool, pAllocator);
}

void BufferData::copy_buffer(const Device& device,
	VkBuffer src_buffer,
	VkBuffer dst_buffer,
	VkDeviceSize size)
{
	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = copy_command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device.logical_device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	VkBufferCopy copy_region {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	auto queue = device.transfer_queue_family.queue;
	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device.logical_device, copy_command_pool, 1, &command_buffer);
}

void BufferData::update_uniform_buffer(
	const VkDevice& device,
	VkExtent2D extent,
	uint32_t current_image)
{
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	UniformBufferObject ubo {};
	ubo.model = glm::rotate(
		glm::mat4(1.0f),
		time * glm::radians(90.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.projection = glm::perspective(
		glm::radians(45.0f),
		((float)extent.width / (float)extent.height),
		0.1f,
		10.0f);
	ubo.projection[1][1] *= -1;

	void* data;
	auto& memory = uniform_buffers[current_image].memory;
	vkMapMemory(device, memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, memory);
}

}
