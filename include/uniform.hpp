#pragma once

#include "buffer.hpp"
#include "frame_data.hpp"
#include "uniform_buffer_object.hpp"

namespace gfx {

struct Device;

// expand this with templates + tuples + dynamic buffers

// where T is a uniform buffer object
struct Uniform {
	per_frame<Buffer> buffer;
	UniformBufferObject ubo;

	void init(const Device& device) {
		auto buffer_size = sizeof(UniformBufferObject);
		for (auto& b : buffer) {
			b.init(
				device,
				buffer_size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}
	}
	void deinit(const Device& device, const VkAllocationCallbacks* pAllocator = nullptr) {
		for (auto& b : buffer)
			b.deinit(device, pAllocator);
	}
};

}
