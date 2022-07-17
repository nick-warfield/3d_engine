#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "constants.hpp"

namespace gfx {

struct Vertex;
struct Device;

struct Mesh {
	std::vector<Vertex> vertices;
	Buffer index_buffer;

	std::vector<uint32_t> indices;
	Buffer vertex_buffer;

	void init(const Device& device);
	void deinit(const Device& device, const VkAllocationCallbacks* = nullptr);
	void update_uniform_buffer(
		const VkDevice& device,
		VkExtent2D extent,
		uint32_t current_image);

private:
	void copy_buffer(const Device& device,
		VkBuffer src_buffer,
		VkBuffer dst_buffer,
		VkDeviceSize size);

	void init_buffers(const Device& device);
	void init_ubo_buffer(const Device& device);
	void load_model();
};


}
