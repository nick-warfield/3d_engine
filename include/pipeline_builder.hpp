#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "context.hpp"
#include "util.hpp"

namespace chch {

VkResult load_shader(const Context* context, const std::string& filename, VkShaderModule* shader_module);

VkPipelineVertexInputStateCreateInfo make_vertex_input_info();
VkPipelineInputAssemblyStateCreateInfo make_input_assembly_info();
VkPipelineViewportStateCreateInfo make_viewport_state_info();
VkPipelineRasterizationStateCreateInfo make_rasterizer_info(
	VkPolygonMode polygon_mode, VkCullModeFlags cull_mode, VkFrontFace front_face);

VkPipelineMultisampleStateCreateInfo make_multisampling_info(VkSampleCountFlagBits samples);
VkPipelineDepthStencilStateCreateInfo make_depth_stencil_info(
	VkBool32 depth_test_enable, VkBool32 depth_write_enable);

VkPipelineColorBlendStateCreateInfo make_color_blending_info();
VkPipelineDynamicStateCreateInfo make_dynamic_state_info();

struct PipelineBuilder {
	struct ShaderInfo {
		std::string filename;
		VkShaderStageFlagBits stage;
	};

	static PipelineBuilder begin(const Context* context)
	{
		PipelineBuilder builder;

		builder.m_context = context;
		builder.m_vertex_input = make_vertex_input_info();
		builder.m_input_assembly = make_input_assembly_info();
		builder.m_viewport_state = make_viewport_state_info();
		builder.m_rasterizer = make_rasterizer_info(
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
			VK_FRONT_FACE_COUNTER_CLOCKWISE);
		builder.m_multisampling = make_multisampling_info(context->msaa_samples);
		builder.m_depth_stencil = make_depth_stencil_info(VK_TRUE, VK_TRUE);
		builder.m_color_blending = make_color_blending_info();
		builder.m_dynamic_state = make_dynamic_state_info();

		return builder;
	}
	VkResult build(VkPipelineLayout* pipeline_layout, VkPipeline* pipeline);

	// Required
	PipelineBuilder shader_stages(std::vector<ShaderInfo> shader_info)
	{
		m_shader_info = shader_info;
		return *this;
	}
	PipelineBuilder descriptor_layouts(std::vector<VkDescriptorSetLayout> layouts)
	{
		m_layouts = layouts;
		return *this;
	}
	PipelineBuilder push_constants(std::vector<VkPushConstantRange> push_constants)
	{
		m_push_constants = push_constants;
		return *this;
	}
	PipelineBuilder render_pass(VkRenderPass render_pass)
	{
		m_render_pass = render_pass;
		return *this;
	}

	// Optional
	PipelineBuilder rasterizer_options(VkPipelineRasterizationStateCreateInfo info)
	{
		m_rasterizer = info;
		return *this;
	}
	PipelineBuilder multisampling_options(VkPipelineMultisampleStateCreateInfo info)
	{
		m_multisampling = info;
		return *this;
	}
	PipelineBuilder depth_stencil_options(VkPipelineDepthStencilStateCreateInfo info)
	{
		m_depth_stencil = info;
		return *this;
	}
	PipelineBuilder vertex_input_options(VkPipelineVertexInputStateCreateInfo info)
	{
		m_vertex_input = info;
		return *this;
	}
	PipelineBuilder input_assembly_options(VkPipelineInputAssemblyStateCreateInfo info)
	{
		m_input_assembly = info;
		return *this;
	}
	PipelineBuilder viewport_state_options(VkPipelineViewportStateCreateInfo info)
	{
		m_viewport_state = info;
		return *this;
	}
	PipelineBuilder color_blending_options(VkPipelineColorBlendStateCreateInfo info)
	{
		m_color_blending = info;
		return *this;
	}
	PipelineBuilder dynamic_state_options(VkPipelineDynamicStateCreateInfo info)
	{
		m_dynamic_state = info;
		return *this;
	}

private:
	void free_shader_mods();

	const Context* m_context;
	std::vector<VkShaderModule> m_shader_mods;
	std::vector<ShaderInfo> m_shader_info;

	VkRenderPass m_render_pass;
	std::vector<VkPipelineShaderStageCreateInfo> m_shader_stages;
	std::vector<VkDescriptorSetLayout> m_layouts;
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
