#pragma once

#include <vulkan/vulkan_core.h>
#include <string>

#include "buffer.hpp"
#include "texture.hpp"
#include "uniform.hpp"

namespace chch {

struct UniformInfo {
	uint32_t binding;
	per_frame<UniformBuffer>* uniform_buffer;
};
struct TextureInfo {
	uint32_t binding;
	Texture* texture;
};

struct Material {
	VkDescriptorPool descriptor_pool;
	per_frame<VkDescriptorSetLayout> descriptor_set_layout;
	per_frame<VkDescriptorSet> descriptor_set;

	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	void init(const Context* context,
			const VkRenderPass& render_pass,
			VkDescriptorSetLayout base_layout,
			std::vector<TextureInfo> texture_info,
			std::vector<UniformInfo> uniform_info,
			std::string vertex_shader_name,
			std::string fragment_shader_name,
			VkCullModeFlagBits cull_mode,
			VkBool32 enable_depth);

	void deinit(const Context* context);
};

}
