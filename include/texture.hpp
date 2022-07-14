#pragma once

#include <vulkan/vulkan_core.h>

namespace gfx {

struct Device;

struct Texture {
	int width, height, channels;
	VkImage image;
	VkImageView image_view;
	VkDeviceMemory image_memory;
	VkSampler sampler;

	void init(const Device& device);
	void deinit(const Device& device, const VkAllocationCallbacks* pAllocator = nullptr);

private:
	void init_texture_view(const Device& device);
	void init_sampler(const Device& device);
};

void transition_image_layout(
	const Device& device,
	VkImage image,
	VkFormat format,
	VkImageLayout old_layout,
	VkImageLayout new_layout);

void copy_buffer_to_image(
	const Device& device,
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height);
}
