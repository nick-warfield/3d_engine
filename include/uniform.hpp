#pragma once

#include "buffer.hpp"
#include "frame_data.hpp"
#include "uniform_buffer_object.hpp"
#include "util.hpp"
#include "device.hpp"

#include <cstring>
#include <iostream>

namespace gfx {

// expand this with templates + tuples + dynamic buffers

// where T is a uniform buffer object

struct Uniform {
	per_frame<Buffer> buffer;
	per_frame<bool> is_stale;

	void* m_ubo;
	VkDeviceSize ubo_size;

	template <typename T>
	T* ubo()
	{
		is_stale.fill(true);
		return (T*)m_ubo;
	}

	template <typename T>
	void init(const Device& device, T* ubo)
	{
		m_ubo = ubo;
		is_stale.fill(true);
		ubo_size = sizeof(T);
		for (size_t i = 0; i < buffer.size(); ++i) {
			buffer[i].init(
				device,
				ubo_size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			update(device.logical_device, i);
		}
	}

	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr)
	{
		for (auto& b : buffer)
			b.deinit(device, pAllocator);
	}

	void update(
		const VkDevice& device,
		uint32_t current_frame)
	{
		if (!is_stale[current_frame])
			return;

		void* data;
		auto& memory = buffer[current_frame].memory;
		vkMapMemory(device, memory, 0, ubo_size, 0, &data);
		memcpy(data, m_ubo, ubo_size);
		vkUnmapMemory(device, memory);

		// caching is not working properly with this jank
		//is_stale[current_frame] = false;
	}
};

}
