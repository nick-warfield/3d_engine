#include "mesh.hpp"
#include "context.hpp"
#include "vertex.hpp"
#include "util.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

namespace chch {

void Mesh::init(const Context* context, std::string filename)
{
	load_model(filename);
	init_buffers(context);
}

void Mesh::init_buffers(const Context* context)
{
	// Create Vertex Buffer
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
	Buffer staging_buffer {};
	staging_buffer.init(context,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(context->allocator, staging_buffer.allocation, &data);
	memcpy(data, vertices.data(), (size_t)buffer_size);
	vmaUnmapMemory(context->allocator, staging_buffer.allocation);

	vertex_buffer.init(context,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	copy_buffer(context, staging_buffer.buffer, vertex_buffer.buffer, buffer_size);

	// Create Index Buffer
	buffer_size = sizeof(indices[0]) * indices.size();
	staging_buffer.deinit(context);
	staging_buffer.init(context,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY);

	vmaMapMemory(context->allocator, staging_buffer.allocation, &data);
	memcpy(data, indices.data(), (size_t)buffer_size);
	vmaUnmapMemory(context->allocator, staging_buffer.allocation);

	index_buffer.init(context,
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	copy_buffer(context, staging_buffer.buffer, index_buffer.buffer, buffer_size);
	staging_buffer.deinit(context);
}

void Mesh::load_model(std::string filename)
{
	tinyobj::attrib_t attribute;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(
			&attribute,
			&shapes,
			&materials,
			&err,
			(Root::path / "resources" / filename).c_str())) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> unique_vertices {};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex {};
			vertex.position = {
				attribute.vertices[3 * index.vertex_index + 0],
				attribute.vertices[3 * index.vertex_index + 1],
				attribute.vertices[3 * index.vertex_index + 2]
			};
			vertex.normal = {
				attribute.normals[3 * index.normal_index + 0],
				attribute.normals[3 * index.normal_index + 1],
				attribute.normals[3 * index.normal_index + 2]
			};
			vertex.uv = {
				attribute.texcoords[2 * index.texcoord_index + 0],
				1.0f - attribute.texcoords[2 * index.texcoord_index + 1]
			};

			if (unique_vertices.count(vertex) == 0) {
				unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(unique_vertices[vertex]);
		}
	}
}

void Mesh::deinit(const Context* context)
{
	index_buffer.deinit(context);
	vertex_buffer.deinit(context);
}

void Mesh::copy_buffer(const Context* context,
	VkBuffer src_buffer,
	VkBuffer dst_buffer,
	VkDeviceSize size)
{
	context->record_transfer_command([size, src_buffer, dst_buffer](VkCommandBuffer command_buffer) {
		VkBufferCopy copy_region {};
		copy_region.srcOffset = 0;
		copy_region.dstOffset = 0;
		copy_region.size = size;
		vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
	});
}

}
