#include "util.hpp"
#include "constants.hpp"
#include "texture.hpp"
#include "uniform.hpp"

#include <fstream>

namespace gfx {

std::vector<char> read_file(const char* filename)
{
	std::ifstream file((Root::path / filename).string(), std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("failed to open file");

	size_t file_size = (size_t)file.tellg();
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
};

VkDescriptorPool make_descriptor_pool(const VkDevice& device)
{
	VkDescriptorPool descriptor_pool;

	std::array<VkDescriptorPoolSize, 2> pool_sizes {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool");

	return descriptor_pool;
}

VkDescriptorSetLayout make_default_descriptor_layout(const VkDevice& device)
{
	VkDescriptorSetLayout set_layout;

	// Create Descriptor Set Layout
	VkDescriptorSetLayoutBinding ubo_layout_binding {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding sampler_layout_binding {};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	sampler_layout_binding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		ubo_layout_binding,
		sampler_layout_binding
	};

	VkDescriptorSetLayoutCreateInfo layout_info {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(
			device,
			&layout_info,
			nullptr,
			&set_layout)
		!= VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout");

	return set_layout;
}

per_frame<VkDescriptorSet> make_descriptor_set(
	const VkDevice& device,
	VkDescriptorPool pool,
	VkDescriptorSetLayout layout,
	const Texture& texture,
	const Uniform& uniform)
{
	per_frame<VkDescriptorSet> d_set;

	// Create Descriptor Sets
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
	VkDescriptorSetAllocateInfo alloc_info;
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = pool;
	alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	alloc_info.pSetLayouts = layouts.data();
	alloc_info.pNext = nullptr;

	if (vkAllocateDescriptorSets(device, &alloc_info, d_set.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets");

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkDescriptorBufferInfo buffer_info {};
		buffer_info.buffer = uniform.buffer[i].buffer;
		buffer_info.offset = 0;
		buffer_info.range = uniform.ubo_size;

		VkDescriptorImageInfo image_info {};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = texture.image.image_view;
		image_info.sampler = texture.sampler;

		std::array<VkWriteDescriptorSet, 2> descriptor_writes {};
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = d_set[i];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &buffer_info;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = d_set[i];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].descriptorCount = 1;
		descriptor_writes[1].pImageInfo = &image_info;

		vkUpdateDescriptorSets(
			device,
			static_cast<uint32_t>(descriptor_writes.size()),
			descriptor_writes.data(),
			0,
			nullptr);
	}

	return d_set;
}

}
