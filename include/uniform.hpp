#pragma once

#include "buffer.hpp"
#include "frame_data.hpp"
#include "util.hpp"
#include "device.hpp"

#include <cstring>
#include <any>
#include <cstring>
#include <iostream>

namespace chch {

struct UniformBuffer {
	Buffer buffer;
	void* data;
	uint32_t ubo_size;

	void init(const Context* context, uint32_t size) {
		ubo_size = size;
		buffer.init(
				context,
				ubo_size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU);
		vmaMapMemory(context->allocator, buffer.allocation, &data);
	}

	void deinit(const Context* context) {
		vmaUnmapMemory(context->allocator, buffer.allocation);
		buffer.deinit(context);
	}

	void copy(void* ubo_data) {
		memcpy(data, ubo_data, ubo_size);
	}
};

template <typename T>
struct Uniform {
	per_frame<UniformBuffer> buffer;

	void init(const Context* context, T ubo) {
		m_ubo = ubo;
		is_stale.fill(true);
		for (size_t i = 0; i < buffer.size(); ++i) {
			buffer[i].init(context, sizeof(T));
			update(i);
		}
	}

	void deinit(const Context* context) {
		for (auto& b : buffer)
			b.deinit(context);
	}

	const T& ubo() const { return m_ubo; }
	T& ubo() { is_stale.fill(true); return m_ubo; }

	void update(uint32_t current_frame)
	{
		if (!is_stale[current_frame])
			return;
		buffer[current_frame].copy((void*)m_ubo);
		is_stale[current_frame] = false;
	}

private:
	per_frame<bool> is_stale;
	T m_ubo;
};

}
