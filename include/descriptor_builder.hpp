#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include "context.hpp"
#include "texture.hpp"
#include "uniform.hpp"
#include "frame_data.hpp"

namespace gfx{ struct Buffer; }

namespace chch {

struct DescriptorBuilder {
	static DescriptorBuilder begin(const Context* context, VkDescriptorPool pool) {
		DescriptorBuilder builder;
		builder.m_context = context;
		builder.m_pool = pool;
		return builder;
	}

	VkResult build(VkDescriptorSetLayout* layout, VkDescriptorSet* set)
	{
		VkDescriptorSetLayoutCreateInfo layout_info {};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(m_bindings.size());
		layout_info.pBindings = m_bindings.data();

		auto result = vkCreateDescriptorSetLayout(
			m_context->device,
			&layout_info,
			m_context->allocation_callbacks,
			layout);

		if (result != VK_SUCCESS)
			return result;

		VkDescriptorSetAllocateInfo alloc_info;
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = m_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = layout;
		alloc_info.pNext = nullptr;

		result = vkAllocateDescriptorSets(m_context->device, &alloc_info, set);
		if (result != VK_SUCCESS)
			return result;

		for (auto& w : m_writes)
			w.dstSet = *set;

		vkUpdateDescriptorSets(
			m_context->device,
			static_cast<uint32_t>(m_writes.size()),
			m_writes.data(),
			0,
			nullptr);

		return result;
	}

	DescriptorBuilder bind_uniform(
			uint32_t binding,
			UniformBuffer* uniform)
	{
		VkDescriptorSetLayoutBinding layout_binding {};
		layout_binding.binding = binding;
		layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layout_binding.descriptorCount = 1;
		layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_binding.pImmutableSamplers = nullptr;
		m_bindings.push_back(layout_binding);

		VkDescriptorBufferInfo info {};
		info.buffer = uniform->buffer.buffer,
		info.offset = 0;
		info.range = uniform->ubo_size;

		VkWriteDescriptorSet layout_write;
		layout_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		layout_write.dstBinding = binding;
		layout_write.dstArrayElement = 0;
		layout_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layout_write.descriptorCount = 1;
		layout_write.pBufferInfo = &info;
		m_writes.push_back(layout_write);

		return *this;
	}

	DescriptorBuilder bind_texture(
			uint32_t binding,
			Texture* texture)
	{
		VkDescriptorImageInfo info {};
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView = texture->image.image_view;
		info.sampler = texture->sampler;

		VkDescriptorSetLayoutBinding layout_binding {};
		layout_binding.binding = binding;
		layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layout_binding.descriptorCount = 1;
		layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_binding.pImmutableSamplers = nullptr;
		m_bindings.push_back(layout_binding);

		VkWriteDescriptorSet layout_write;
		layout_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		layout_write.dstBinding = binding;
		layout_write.dstArrayElement = 0;
		layout_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layout_write.descriptorCount = 1;
		layout_write.pImageInfo = &info;

		m_writes.push_back(layout_write);

		return *this;
	}

private:
	const Context* m_context;
	VkDescriptorPool m_pool;
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	std::vector<VkWriteDescriptorSet> m_writes;
};

}
