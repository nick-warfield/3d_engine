#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include "context.hpp"

namespace chch {

struct DescriptorBuilder {
	static DescriptorBuilder begin(const Context* context, VkDescriptorPool pool) {
		DescriptorBuilder builder;
		builder.m_context = context;
		builder.m_pool = pool;
		return builder;
	}

	VkResult build(VkDescriptorSetLayout& layout, VkDescriptorSet& set);

	DescriptorBuilder bind_buffer(
			uint32_t binding,
			VkDescriptorBufferInfo* info,
			VkDescriptorType type,
			VkShaderStageFlagBits stages);

	DescriptorBuilder bind_image(
			uint32_t binding,
			VkDescriptorImageInfo* info,
			VkDescriptorType type,
			VkShaderStageFlagBits stages);

private:
	const Context* m_context;
	VkDescriptorPool m_pool;
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	std::vector<VkWriteDescriptorSet> m_writes;
};

}
