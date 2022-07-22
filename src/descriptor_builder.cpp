#include "descriptor_builder.hpp"

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

DescriptorBuilder DescriptorBuilder::bind_buffer(
	uint32_t binding,
	VkDescriptorBufferInfo* info,
	VkDescriptorType type,
	VkShaderStageFlagBits stages)
{
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
	layout_write.pBufferInfo = info;
	m_writes.push_back(layout_write);

	return *this;
}

DescriptorBuilder DescriptorBuilder::bind_image(
	uint32_t binding,
	VkDescriptorImageInfo* info,
	VkDescriptorType type,
	VkShaderStageFlagBits stages)
{
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
	layout_write.pImageInfo = info;
	m_writes.push_back(layout_write);

	return *this;
}

}
