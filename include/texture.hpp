#pragma once

#include "vk_mem_alloc.h"
#include <vulkan/vulkan_core.h>
#include <vector>
#include <string>

namespace chch {

struct Context;

struct Image {
	VkImage image;
	VkImageView image_view;
	VmaAllocation allocation;

	void init(const Context* context, uint32_t width, uint32_t height, uint32_t mip_levels,
			VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling,
			VkImageAspectFlags aspect_flags, VkImageUsageFlags image_usage,
			VkMemoryPropertyFlags memory_properties, VmaMemoryUsage memory_usage);
	void deinit(const Context* context);
};

struct Texture {
	uint32_t width, height;
	uint32_t channels;
	uint32_t mip_levels;

	VkSampler sampler;
	Image image;

	void init(const Context* context, std::string filename);
	void deinit(const Context* context);

private:
	void init_texture(const Context* context, std::string filename);
	void init_sampler(const Context* context);
};

}
