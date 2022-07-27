#include "descriptor_builder.hpp"
#include "texture.hpp"
#include "uniform.hpp"

namespace chch {

VkResult DescriptorBuilder::build(VkDescriptorSetLayout& layout, VkDescriptorSet& set)
{
	VkDescriptorSetLayoutCreateInfo layout_info {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(m_bindings.size());
	layout_info.pBindings = m_bindings.data();

	auto result = vkCreateDescriptorSetLayout(
		m_context->device,
		&layout_info,
		m_context->allocation_callbacks,
		&layout);

	if (result != VK_SUCCESS)
		return result;

	VkDescriptorSetAllocateInfo alloc_info;
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &layout;
	alloc_info.pNext = nullptr;

	result = vkAllocateDescriptorSets(m_context->device, &alloc_info, &set);
	if (result != VK_SUCCESS)
		return result;

	for (auto& w : m_writes)
		w.dstSet = set;

	vkUpdateDescriptorSets(
		m_context->device,
		static_cast<uint32_t>(m_writes.size()),
		m_writes.data(),
		0,
		nullptr);

	return result;
}

template <typename T>
DescriptorBuilder DescriptorBuilder::bind_buffer(
	uint32_t binding,
	Uniform<T>* uniform,
	VkDescriptorType type,
	VkShaderStageFlagBits stages)
{
	VkDescriptorBufferInfo info {};
	info.buffer = uniform->buffer[0].buffer,
	info.offset = 0;
	info.range = sizeof(T);

	VkDescriptorSetLayoutBinding layout_binding {};
	layout_binding.binding = binding;
	layout_binding.descriptorType = type;
	layout_binding.descriptorCount = 1;
	layout_binding.stageFlags = stages;
	layout_binding.pImmutableSamplers = nullptr;
	m_bindings.push_back(layout_binding);

	VkWriteDescriptorSet layout_write;
	layout_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	layout_write.dstBinding = binding;
	layout_write.dstArrayElement = 0;
	layout_write.descriptorType = type;
	layout_write.descriptorCount = 1;
	layout_write.pBufferInfo = &info;
	m_writes.push_back(layout_write);

	return *this;
}

DescriptorBuilder DescriptorBuilder::bind_image(
	uint32_t binding,
	Texture* texture,
	VkDescriptorType type,
	VkShaderStageFlagBits stages)
{
	VkDescriptorImageInfo info {};
	info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	info.imageView = texture->image.image_view;
	info.sampler = texture->sampler;

	VkDescriptorSetLayoutBinding layout_binding {};
	layout_binding.binding = binding;
	layout_binding.descriptorType = type;
	layout_binding.descriptorCount = 1;
	layout_binding.stageFlags = stages;
	layout_binding.pImmutableSamplers = nullptr;
	m_bindings.push_back(layout_binding);

	VkWriteDescriptorSet layout_write;
	layout_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	layout_write.dstBinding = binding;
	layout_write.dstArrayElement = 0;
	layout_write.descriptorType = type;
	layout_write.descriptorCount = 1;
	layout_write.pImageInfo = &info;
	m_writes.push_back(layout_write);

	return *this;
}

}
