#pragma once

#include "buffer.hpp"
#include "frame_data.hpp"
#include "uniform_buffer_object.hpp"

#define GLM_FORCE_RADIANS
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <chrono>
#include <cstring>

namespace gfx {

struct Device;

// expand this with templates + tuples + dynamic buffers

// where T is a uniform buffer object
struct Uniform {
	per_frame<Buffer> buffer;
	UniformBufferObject ubo;

	void init(const Device& device)
	{
		auto buffer_size = sizeof(UniformBufferObject);
		for (auto& b : buffer) {
			b.init(
				device,
				buffer_size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}
	}

	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr)
	{
		for (auto& b : buffer)
			b.deinit(device, pAllocator);
	}

	void update(
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
		auto& memory = buffer[current_image].memory;
		vkMapMemory(device, memory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, memory);
	}
};

}
