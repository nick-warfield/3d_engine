#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>

namespace gfx {

struct Device;

struct Image {
	VkImage image;
	VkImageView image_view;
	VkDeviceMemory image_memory;

	void init(const Device &device, int width, int height, VkSampleCountFlagBits samples,
			VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties, VkImageAspectFlags aspect_flags);
	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr);
};

struct Texture {
	int width, height, channels;
	VkSampler sampler;
	Image image;

	void init(const Device& device);
	void deinit(const Device& device, const VkAllocationCallbacks* pAllocator = nullptr);

private:
	void init_texture(const Device& device);
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

VkFormat find_supported_format(
	const Device& device,
	const std::vector<VkFormat>& candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features);
}