#include "pipeline_builder.hpp"
#include "context.hpp"
#include "util.hpp"

// update these later
#include "vertex.hpp"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace chch {

VkResult PipelineBuilder::load_shader(const Context* context, const std::string& filename, VkShaderModule* shader_module)
{
	auto shader_file = chch::read_file(("shaders/" + filename).c_str());

	VkShaderModuleCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader_file.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(shader_file.data());

	return vkCreateShaderModule(
		context->device,
		&create_info,
		context->allocation_callbacks,
		shader_module);
}

// Required
PipelineBuilder PipelineBuilder::add_shader(const std::string& filename, VkShaderStageFlagBits stage)
{
	m_shader_info.push_back({ filename, stage });
	return *this;
}

PipelineBuilder PipelineBuilder::add_layout(uint32_t set_number, VkDescriptorSetLayout layout)
{
	m_layouts[set_number] = layout;
	return *this;
}

PipelineBuilder PipelineBuilder::set_render_pass(VkRenderPass render_pass)
{
	m_render_pass = render_pass;
	return *this;
}

PipelineBuilder PipelineBuilder::add_push_constant(uint32_t offset, uint32_t size, VkShaderStageFlagBits stages)
{
	VkPushConstantRange push_constant {};
	push_constant.offset = offset;
	push_constant.size = size;
	push_constant.stageFlags = stages;
	m_push_constants.push_back(push_constant);
	return *this;
}

PipelineBuilder PipelineBuilder::set_vertex_input()
{
	auto binding_description = chch::Vertex::get_binding_description();
	auto attribute_description = chch::Vertex::get_attribute_description();

	VkPipelineVertexInputStateCreateInfo vertex_input_info {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_description.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_description.data();

	m_vertex_input = vertex_input_info;
	return *this;
}

PipelineBuilder PipelineBuilder::set_input_assembly()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	m_input_assembly = input_assembly;
	return *this;
}

PipelineBuilder PipelineBuilder::set_viewport_state()
{
	VkPipelineViewportStateCreateInfo viewport_state {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	m_viewport_state = viewport_state;
	return *this;
}

PipelineBuilder PipelineBuilder::set_rasterizer(
	VkPolygonMode polygon_mode,
	VkCullModeFlags cull_mode,
	VkFrontFace front_face)
{
	VkPipelineRasterizationStateCreateInfo rasterizer {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = polygon_mode;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = cull_mode;
	rasterizer.frontFace = front_face;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	m_rasterizer = rasterizer;
	return *this;
}

PipelineBuilder PipelineBuilder::set_multisampling(VkSampleCountFlagBits samples)
{
	VkPipelineMultisampleStateCreateInfo multisampling {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = samples;
	multisampling.minSampleShading = 0.2f;
	//	multisampling.pSampleMask = nullptr;
	//	multisampling.alphaToCoverageEnable = VK_FALSE;
	//	multisampling.alphaToOneEnable = VK_FALSE;

	m_multisampling = multisampling;
	return *this;
}

PipelineBuilder PipelineBuilder::set_depth_stencil(
	VkBool32 depth_test_enable,
	VkBool32 depth_write_enable)
{
	VkPipelineDepthStencilStateCreateInfo depth_stencil {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = depth_test_enable;
	depth_stencil.depthWriteEnable = depth_write_enable;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.minDepthBounds = 0.0f;
	depth_stencil.maxDepthBounds = 1.0f;
	depth_stencil.stencilTestEnable = VK_FALSE;
	depth_stencil.front = {};
	depth_stencil.back = {};

	m_depth_stencil = depth_stencil;
	return *this;
}

PipelineBuilder PipelineBuilder::set_color_blending()
{
	VkPipelineColorBlendAttachmentState color_blend_attachment {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	//	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	//	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	//	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	//	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	m_color_blending = color_blending;
	return *this;
}

PipelineBuilder PipelineBuilder::set_dynamic_state()
{
	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	m_dynamic_state = dynamic_state;
	return *this;
}

void PipelineBuilder::free_shader_mods()
{
	for (auto& mod : m_shader_mods)
		vkDestroyShaderModule(m_context->device, mod, m_context->allocation_callbacks);
}

VkResult PipelineBuilder::build(VkPipelineLayout* pipeline_layout, VkPipeline* pipeline)
{
	m_shader_mods.resize(m_shader_info.size());
	m_shader_stages.resize(m_shader_info.size());
	for (auto& shader : m_shader_info) {
		VkShaderModule mod;
		auto result = load_shader(m_context, shader.filename, &mod);
		if (result != VK_SUCCESS)
			free_shader_mods();

		VkPipelineShaderStageCreateInfo shader_stage_info {};
		shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_info.stage = shader.stage;
		shader_stage_info.module = mod;
		shader_stage_info.pName = "main";

		m_shader_mods.push_back(mod);
		m_shader_stages.push_back(shader_stage_info);
	}

	std::vector<VkDescriptorSetLayout> layouts(m_layouts.begin(), m_layouts.end());
	VkPipelineLayoutCreateInfo pipeline_layout_info {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = layouts.size();
	pipeline_layout_info.pSetLayouts = layouts.data();
	pipeline_layout_info.pushConstantRangeCount = m_push_constants.size();
	pipeline_layout_info.pPushConstantRanges = m_push_constants.data();

	auto result = vkCreatePipelineLayout(
		m_context->device,
		&pipeline_layout_info,
		m_context->allocation_callbacks,
		pipeline_layout);

	if (result != VK_SUCCESS) {
		free_shader_mods();
		return result;
	}

	VkGraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = m_shader_stages.size();
	pipeline_info.pStages = m_shader_stages.data();
	pipeline_info.pVertexInputState = &m_vertex_input;
	pipeline_info.pInputAssemblyState = &m_input_assembly;
	pipeline_info.pViewportState = &m_viewport_state;
	pipeline_info.pRasterizationState = &m_rasterizer;
	pipeline_info.pMultisampleState = &m_multisampling;
	pipeline_info.pDepthStencilState = &m_depth_stencil;
	pipeline_info.pColorBlendState = &m_color_blending;
	pipeline_info.pDynamicState = &m_dynamic_state;
	pipeline_info.layout = *pipeline_layout;
	pipeline_info.renderPass = m_render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines(
		m_context->device,
		VK_NULL_HANDLE,
		1,
		&pipeline_info,
		m_context->allocation_callbacks,
		pipeline);

	free_shader_mods();
	return result;
}

}
