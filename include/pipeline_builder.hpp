#pragma once

#include <string>
#include <map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "context.hpp"
#include "util.hpp"

namespace chch {

struct PipelineBuilder {
	static PipelineBuilder begin(const Context* context)
	{
		PipelineBuilder builder;
		builder.m_context = context;
		builder.set_vertex_input();
		builder.set_input_assembly();
		builder.set_viewport_state();
		builder.set_rasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		builder.set_multisampling(context->msaa_samples);
		builder.set_depth_stencil(VK_TRUE, VK_TRUE);
		builder.set_color_blending();
		builder.set_dynamic_state();
		return builder;
	}
	VkResult build(VkPipelineLayout* pipeline_layout, VkPipeline* pipeline);

	// Required
	PipelineBuilder add_shader(const std::string& filename, VkShaderStageFlagBits stage);
	PipelineBuilder add_layout(uint32_t set_number, VkDescriptorSetLayout layout);
	PipelineBuilder set_render_pass(VkRenderPass render_pass);

	// Optional
	PipelineBuilder add_push_constant(uint32_t offset, uint32_t size, VkShaderStageFlagBits stages);
	PipelineBuilder set_vertex_input();
	PipelineBuilder set_input_assembly();
	PipelineBuilder set_viewport_state();
	PipelineBuilder set_color_blending();
	PipelineBuilder set_dynamic_state();
	PipelineBuilder set_multisampling(VkSampleCountFlagBits samples);
	PipelineBuilder set_depth_stencil(VkBool32 depth_test_enable, VkBool32 depth_write_enable);
	PipelineBuilder set_rasterizer(
			VkPolygonMode polygon_mode,
			VkCullModeFlags cull_mode,
			VkFrontFace front_face);

private:
	void free_shader_mods();
	VkResult load_shader(const Context* context, const std::string& filename, VkShaderModule* shader_module);

	struct ShaderInfo {
		std::string filename;
		VkShaderStageFlagBits stage;
	};

	std::map<uint32_t, VkDescriptorSetLayout> m_layouts;
	const Context* m_context;
	VkRenderPass m_render_pass;
	std::vector<VkShaderModule> m_shader_mods;
	std::vector<ShaderInfo> m_shader_info;
	std::vector<VkPipelineShaderStageCreateInfo> m_shader_stages;
	std::vector<VkPushConstantRange> m_push_constants;
	VkPipelineVertexInputStateCreateInfo m_vertex_input;
	VkPipelineInputAssemblyStateCreateInfo m_input_assembly;
	VkPipelineViewportStateCreateInfo m_viewport_state;
	VkPipelineRasterizationStateCreateInfo m_rasterizer;
	VkPipelineMultisampleStateCreateInfo m_multisampling;
	VkPipelineDepthStencilStateCreateInfo m_depth_stencil;
	VkPipelineColorBlendStateCreateInfo m_color_blending;
	VkPipelineDynamicStateCreateInfo m_dynamic_state;
};

}
