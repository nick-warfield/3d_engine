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
	Texture texture;
	Uniform uniform;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	per_frame<VkDescriptorSet> descriptor_set;

	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	template <typename T>
	void init(const Device& device,
			const VkRenderPass& render_pass,
			VkDescriptorSetLayout base_layout,
			T* ubo,
			std::string texture_name,
			std::string vertex_shader_name,
			std::string fragment_shader_name)
	{
		auto vert_shader = load_shader(device.logical_device, vertex_shader_name);
		auto frag_shader = load_shader(device.logical_device, fragment_shader_name);

		uniform.init(device, ubo);
		texture.init(device, texture_name);

		init_descriptor_set(device.logical_device);
		init_pipeline(device, render_pass, base_layout, vert_shader, frag_shader);

		vkDestroyShaderModule(device.logical_device, vert_shader, nullptr);
		vkDestroyShaderModule(device.logical_device, frag_shader, nullptr);
	}

	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr);

private:
	void init_descriptor_set(const VkDevice& device);
	void init_pipeline(
			const Device& device,
			VkRenderPass render_pass,
			VkDescriptorSetLayout base_layout,
			VkShaderModule vert_shader,
			VkShaderModule frag_shader);
};

}
