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
			const VkPipeline& base_pipeline,
			std::string vertex_shader_name,
			std::string fragment_shader_name);
	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr);

	static Material make_base_material(
			const Device& device,
			VkRenderPass render_pass,
			std::string vertex_shader_name,
			std::string fragment_shader_name)
	{
		Material m;
		auto vert_shader =
			load_shader(device.logical_device, vertex_shader_name);
		auto frag_shader =
			load_shader(device.logical_device, fragment_shader_name);

		m.init_descriptor_set(device.logical_device);
		m.init_base_pipeline(device, render_pass, vert_shader, frag_shader);

		vkDestroyShaderModule(device.logical_device, vert_shader, nullptr);
		vkDestroyShaderModule(device.logical_device, frag_shader, nullptr);

		return m;
	}

private:
	void init_descriptor_set(const VkDevice& device);
	void init_derived_pipeline(
			const Device& device,
			const VkPipeline& base_pipeline,
			VkShaderModule vert_shader,
			VkShaderModule frag_shader);
	void init_base_pipeline(
			const Device& device,
			VkRenderPass render_pass,
			VkShaderModule vert_shader,
			VkShaderModule frag_shader);
};

}
