#include "util.hpp"
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "texture.hpp"
#include "context.hpp"

namespace chch {

void Image::init(const Context* context, uint32_t width, uint32_t height, uint32_t mip_levels,
	VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling,
	VkImageAspectFlags aspect_flags, VkImageUsageFlags image_usage,
	VkMemoryPropertyFlags memory_properties, VmaMemoryUsage memory_usage)
{
	VkImageCreateInfo image_info {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = static_cast<uint32_t>(width);
	image_info.extent.height = static_cast<uint32_t>(height);
	image_info.extent.depth = 1;
	image_info.mipLevels = mip_levels;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = image_usage;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = samples;
	image_info.flags = 0;

	VmaAllocationCreateInfo alloc_info {};
	alloc_info.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
	alloc_info.usage = memory_usage;
	alloc_info.requiredFlags = memory_properties;
	alloc_info.memoryTypeBits = 0;

	auto result = vmaCreateImage(context->allocator, &image_info, &alloc_info, &image, &allocation, nullptr);
	vk_check(result, "Failed to create image");

	VkImageViewCreateInfo view_info {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;

	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = mip_levels;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	result = vkCreateImageView(context->device, &view_info, nullptr, &image_view);
	vk_check(result, "Failed to create image view");
}

void Image::deinit(const Context* context)
{
	vkDestroyImageView(context->device, image_view, context->allocation_callbacks);
	vmaDestroyImage(context->allocator, image, allocation);
}

void Texture::init(const Context* context, std::string filename)
{
	init_texture(context, filename);
	init_sampler(context);
}

void Texture::deinit(const Context* context)
{
	vkDestroySampler(context->device, sampler, context->allocation_callbacks);
	image.deinit(context);
}

void Texture::init_texture(const Context* context, std::string filename)
{
	int f_width, f_height, f_channels;
	stbi_uc* pixels = stbi_load(
		(Root::path / "resources" / filename).c_str(),
		&f_width,
		&f_height,
		&f_channels,
		STBI_rgb_alpha);
	VkDeviceSize size = f_width * f_height * 4;

	if (!pixels)
		throw std::runtime_error("failed to load texture image");

	width = static_cast<uint32_t>(f_width);
	height = static_cast<uint32_t>(f_height);
	channels = static_cast<uint32_t>(f_channels);
	mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	Buffer staging_buffer;
	staging_buffer.init(
		context,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(context->allocator, staging_buffer.allocation, &data);
	memcpy(data, pixels, static_cast<size_t>(size));
	vmaUnmapMemory(context->allocator, staging_buffer.allocation);

	stbi_image_free(pixels);
	image.init(
		context,
		width,
		height,
		mip_levels,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	transition_image_layout(
		context,
		image.image,
		mip_levels,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_to_image(
		context,
		staging_buffer.buffer,
		image.image,
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height));

	// generate_mipmaps transitions to fragment shader layout for us
	generate_mipmaps(context, image.image, VK_FORMAT_R8G8B8A8_SRGB, width, height, mip_levels);
	staging_buffer.deinit(context);
}

void Texture::init_sampler(const Context* context)
{
	VkSamplerCreateInfo sampler_info {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties {};
	vkGetPhysicalDeviceProperties(context->physical_device, &properties);
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = static_cast<float>(mip_levels);
	sampler_info.mipLodBias = 0.0f;

	auto result = vkCreateSampler(context->device, &sampler_info, context->allocation_callbacks, &sampler);
	vk_check(result, "Failed to create sampler");
}

}
