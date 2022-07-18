#pragma once

#include <vulkan/vulkan_core.h>
#include <string>

#include "device.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "uniform.hpp"

namespace gfx {

VkShaderModule load_shader(VkDevice device, std::string filename);

struct Material {
	Uniform uniform;
	Texture texture;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	per_frame<VkDescriptorSet> descriptor_set;

	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	void init(const Device& device,
			const VkRenderPass& render_pass,
			std::string texture_name,
			std::string vertex_shader_name,
			std::string fragment_shader_name);
	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr);

private:
	void init_descriptor_set(const VkDevice& device);
	void init_pipeline(
			const Device& device,
			VkRenderPass render_pass,
			VkShaderModule vert_shader,
			VkShaderModule frag_shader);
};

}
