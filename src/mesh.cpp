#include "mesh.hpp"
#include "constants.hpp"
#include "device.hpp"
#include "uniform_buffer_object.hpp"
#include "vertex.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

namespace gfx {

void Mesh::init(const Device& device)
{
	load_model();
	init_buffers(device);
}

void Mesh::init_buffers(const Device& device)
{
	// Create Vertex Buffer
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
	Buffer staging_buffer {};
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
	staging_buffer.deinit(device.logical_device);
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
	staging_buffer.deinit(device.logical_device);
}

void Mesh::load_model()
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
			(Root::path / MODEL_PATH).c_str())) {
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
			vertex.tex_coord = {
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

void Mesh::deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator)
{
	index_buffer.deinit(device, pAllocator);
	vertex_buffer.deinit(device, pAllocator);
}

void Mesh::copy_buffer(const Device& device,
	VkBuffer src_buffer,
	VkBuffer dst_buffer,
	VkDeviceSize size)
{
	device.record_transfer_commands([size, src_buffer, dst_buffer](VkCommandBuffer command_buffer) {
		VkBufferCopy copy_region {};
		copy_region.srcOffset = 0;
		copy_region.dstOffset = 0;
		copy_region.size = size;
		vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
	});
}

}
