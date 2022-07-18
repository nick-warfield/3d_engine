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
#include "constants.hpp"
#include "device.hpp"
#include "texture.hpp"

namespace gfx {

void Image::init(const Device& device, uint32_t width, uint32_t height, uint32_t mip_levels,
	VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling,
	VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
	VkImageAspectFlags aspect_flags)
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
	image_info.usage = usage;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = samples;
	image_info.flags = 0;

	if (vkCreateImage(device.logical_device, &image_info, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("failed to create image");

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device.logical_device, image, &memory_requirements);

	VkMemoryAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = device.find_memory_type(
		memory_requirements.memoryTypeBits,
		properties);

	if (vkAllocateMemory(device.logical_device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory");

	vkBindImageMemory(device.logical_device, image, image_memory, 0);

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

	if (vkCreateImageView(device.logical_device, &view_info, nullptr, &image_view) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture image view");
}

void Image::deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator)
{
	vkDestroyImageView(device, image_view, pAllocator);
	vkDestroyImage(device, image, pAllocator);
	vkFreeMemory(device, image_memory, pAllocator);
}

void Texture::init(const Device& device)
{
	init_texture(device);
	init_sampler(device);
}

void Texture::deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator)
{
	vkDestroySampler(device, sampler, pAllocator);
	image.deinit(device, pAllocator);
}

void Texture::init_texture(const Device& device)
{
	int f_width, f_height, f_channels;
	stbi_uc* pixels = stbi_load(
		(Root::path / TEXTURE_PATH).c_str(),
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
		device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(device.logical_device, staging_buffer.memory, 0, size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(size));
	vkUnmapMemory(device.logical_device, staging_buffer.memory);

	stbi_image_free(pixels);
	image.init(
		device,
		width,
		height,
		mip_levels,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT);

	transition_image_layout(
		device,
		image.image,
		mip_levels,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_to_image(
		device,
		staging_buffer.buffer,
		image.image,
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height));

	// generate_mipmaps transitions to fragment shader layout for us
	generate_mipmaps(device, image.image, VK_FORMAT_R8G8B8A8_SRGB, width, height, mip_levels);
	staging_buffer.deinit(device.logical_device);
}

void Texture::init_sampler(const Device& device)
{
	VkSamplerCreateInfo sampler_info {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties {};
	vkGetPhysicalDeviceProperties(device.physical_device, &properties);
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

	if (vkCreateSampler(device.logical_device, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler");
}

void transition_image_layout(
	const Device& device,
	VkImage image,
	uint32_t mip_levels,
	VkFormat format,
	VkImageLayout old_layout,
	VkImageLayout new_layout)
{
	device.record_graphics_commands(
		[image, format, mip_levels, old_layout, new_layout](VkCommandBuffer command_buffer) {
			(void)format;

			VkImageMemoryBarrier barrier {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = old_layout;
			barrier.newLayout = new_layout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;

			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = mip_levels;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;

			VkPipelineStageFlags source_stage;
			VkPipelineStageFlags destination_stage;

			if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED
				&& new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				&& new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} else {
				throw std::invalid_argument("unsupported layout transition");
			}

			vkCmdPipelineBarrier(
				command_buffer,
				source_stage, destination_stage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		});
}

void copy_buffer_to_image(
	const Device& device,
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height)
{
	device.record_transfer_commands([buffer, image, width, height](VkCommandBuffer command_buffer) {
		VkBufferImageCopy region {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			command_buffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
	});
}

VkFormat find_supported_format(
	const Device& device,
	const std::vector<VkFormat>& candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features)
{
	for (auto format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device.physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR
			&& (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL
			&& (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format");
}

void generate_mipmaps(
	const Device& device,
	VkImage image,
	VkFormat format,
	int32_t width,
	int32_t height,
	uint32_t mip_levels)
{
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(device.physical_device, format, &props);
	if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("texture image format does not support linear blitting");

	device.record_graphics_commands([image, width, height, mip_levels](VkCommandBuffer command_buffer) {
		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		auto m_width = width;
		auto m_height = height;
		for (uint32_t i = 1; i < mip_levels; ++i) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
				command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { m_width, m_height, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = {
				m_width > 1 ? m_width / 2 : 1,
				m_height > 1 ? m_height / 2 : 1,
				1
			};
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(
				command_buffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (m_width > 1) m_width /= 2;
			if (m_height > 1) m_height /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mip_levels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	});
}

}
