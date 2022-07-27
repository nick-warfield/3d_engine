#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include "context.hpp"
#include "texture.hpp"
#include "uniform.hpp"
#include "frame_data.hpp"

namespace chch {

struct DescriptorBuilder {
	static DescriptorBuilder begin(const Context* context, VkDescriptorPool pool) {
		DescriptorBuilder builder;
		builder.m_context = context;
		builder.m_pool = pool;
		return builder;
	}

	VkResult build(VkDescriptorSetLayout* layout, VkDescriptorSet* set);
	DescriptorBuilder bind_uniform(uint32_t binding, UniformBuffer* uniform);
	DescriptorBuilder bind_texture(uint32_t binding, Texture* texture);

private:
	const Context* m_context;
	VkDescriptorPool m_pool;
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	std::vector<VkWriteDescriptorSet> m_writes;
};

}
