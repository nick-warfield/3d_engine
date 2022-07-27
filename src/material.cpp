#include "material.hpp"
#include "util.hpp"
#include "vertex.hpp"
#include "context.hpp"

#include "descriptor_builder.hpp"
#include "pipeline_builder.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace chch {

void Material::init(const Context* context,
		const VkRenderPass& render_pass,
		VkDescriptorSetLayout base_layout,
		std::vector<Texture*> texture,
		std::vector<per_frame<UniformBuffer>*> uniform,
		std::string vertex_shader_name,
		std::string fragment_shader_name,
		VkCullModeFlagBits cull_mode,
		VkBool32 enable_depth)
{
	descriptor_pool = make_descriptor_pool(context->device, texture.size(), uniform.size());

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		auto builder = DescriptorBuilder::begin(context, descriptor_pool);
		uint32_t binding = 0;
		for (auto& t : texture) {
			builder.bind_texture(binding, t);
			binding++;
		}
		for (auto& u : uniform) {
			builder.bind_uniform(binding, &u->at(i));
			binding++;
		}
		builder.build(&descriptor_set_layout[i], &descriptor_set[i]);
	}

	PipelineBuilder::begin(context)
		.add_shader(vertex_shader_name, VK_SHADER_STAGE_VERTEX_BIT)
		.add_shader(fragment_shader_name, VK_SHADER_STAGE_FRAGMENT_BIT)
		.set_render_pass(render_pass)
		.add_layout(0, base_layout)
		.add_layout(1, descriptor_set_layout[0])
		.add_push_constant(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
		.set_depth_stencil(enable_depth, enable_depth)
		.set_rasterizer(VK_POLYGON_MODE_FILL, cull_mode, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.build(&pipeline_layout, &pipeline);
}


void Material::deinit(const Context* context)
{
	vkDestroyPipeline(context->device, pipeline, context->allocation_callbacks);
	vkDestroyPipelineLayout(context->device, pipeline_layout, context->allocation_callbacks);
	vkDestroyDescriptorPool(context->device, descriptor_pool, context->allocation_callbacks);
	for (auto& layout : descriptor_set_layout)
		vkDestroyDescriptorSetLayout(context->device, layout, context->allocation_callbacks);
}


}
