#pragma once

#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "texture.hpp"
#include "uniform.hpp"
#include "uniform_buffer_object.hpp"

namespace gfx {

struct Material {
	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	per_frame<VkDescriptorSet> descriptor_set;

	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	void init(const Device& device, const Uniform& uniform, const Texture& texture);
	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr);

private:
	void init_descriptor_set();
	void init_pipeline();
};

}
