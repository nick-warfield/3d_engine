#include "material.hpp"
#include "device.hpp"
#include "util.hpp"
#include "vertex.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace gfx {

void Material::init(
	const Device& device,
	const VkRenderPass& render_pass,
	VkDescriptorSetLayout base_layout,
	std::string texture_name,
	std::string vertex_shader_name,
	std::string fragment_shader_name)
{
	auto vert_shader = load_shader(device.logical_device, vertex_shader_name);
	auto frag_shader = load_shader(device.logical_device, fragment_shader_name);

	uniform.init(device);
	texture.init(device, texture_name);

	init_descriptor_set(device.logical_device);
	init_pipeline(device, render_pass, base_layout, vert_shader, frag_shader);

	vkDestroyShaderModule(device.logical_device, vert_shader, nullptr);
	vkDestroyShaderModule(device.logical_device, frag_shader, nullptr);
}

void Material::deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator)
{
	vkDestroyPipeline(device, pipeline, pAllocator);
	vkDestroyPipelineLayout(device, pipeline_layout, pAllocator);
	vkDestroyDescriptorPool(device, descriptor_pool, pAllocator);
	vkDestroyDescriptorSetLayout(device, descriptor_set_layout, pAllocator);
	texture.deinit(device);
	uniform.deinit(device);
}

void Material::init_descriptor_set(const VkDevice& device)
{
	descriptor_pool = make_descriptor_pool(device);
	descriptor_set_layout = make_default_descriptor_layout(device);
	descriptor_set = make_descriptor_set(
		device,
		descriptor_pool,
		descriptor_set_layout,
		texture,
		uniform);
}

void Material::init_pipeline(
	const Device& device,
	VkRenderPass render_pass,
	VkDescriptorSetLayout base_layout,
	VkShaderModule vert_shader,
	VkShaderModule frag_shader)
{
	VkPipelineShaderStageCreateInfo vert_shader_stage_info {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		vert_shader_stage_info,
		frag_shader_stage_info
	};

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	auto binding_description = Vertex::get_binding_description();
	auto attribute_description = Vertex::get_attribute_description();

	VkPipelineVertexInputStateCreateInfo vertex_input_info {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_description.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_description.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = device.msaa_samples;
	multisampling.minSampleShading = 0.2f;
	//	multisampling.pSampleMask = nullptr;
	//	multisampling.alphaToCoverageEnable = VK_FALSE;
	//	multisampling.alphaToOneEnable = VK_FALSE;

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

	VkPipelineDepthStencilStateCreateInfo depth_stencil {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.minDepthBounds = 0.0f;
	depth_stencil.maxDepthBounds = 1.0f;
	depth_stencil.stencilTestEnable = VK_FALSE;
	depth_stencil.front = {};
	depth_stencil.back = {};

	VkDescriptorSetLayout layouts[] {
		base_layout,            // global set
		descriptor_set_layout   // instance set
	};

	VkPushConstantRange push_constant_range {};
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(glm::mat4);
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo pipeline_layout_info {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2;
	pipeline_layout_info.pSetLayouts = layouts;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	if (vkCreatePipelineLayout(device.logical_device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout");

	VkGraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = &dynamic_state;
	pipeline_info.layout = pipeline_layout;
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device.logical_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline)
		!= VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline");
}

VkShaderModule load_shader(VkDevice device, std::string filename)
{
	auto shader_file = read_file(("shaders/" + filename).c_str());

	VkShaderModuleCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader_file.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(shader_file.data());

	VkShaderModule shader_module {};
	if (vkCreateShaderModule(
			device,
			&create_info,
			nullptr,
			&shader_module)
		!= VK_SUCCESS)
		throw std::runtime_error("failed to create shader module");

	return shader_module;
}

}
