#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <string>

#include "buffer.hpp"
#include "vertex.hpp"

namespace chch {

struct Context;

struct Mesh {
	std::vector<Vertex> vertices;
	Buffer index_buffer;

	std::vector<uint32_t> indices;
	Buffer vertex_buffer;

	void init(const Context* context, std::string filename);
	void deinit(const Context* context);

private:
	void copy_buffer(const Context* context,
		VkBuffer src_buffer,
		VkBuffer dst_buffer,
		VkDeviceSize size);

	void init_buffers(const Context* context);
	void load_model(std::string filename);
};


}
