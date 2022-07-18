#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "frame_data.hpp"

namespace gfx {

struct Texture;
struct Uniform;

std::vector<char> read_file(const char* filename);

// Add Shader Introspection Later
VkDescriptorPool make_descriptor_pool(const VkDevice& device);
VkDescriptorSetLayout make_default_descriptor_layout(const VkDevice& device);
per_frame<VkDescriptorSet> make_descriptor_set(
	const VkDevice& device,
	VkDescriptorPool pool,
	VkDescriptorSetLayout layout,
	const Texture& texture,
	const Uniform& uniform);
}
