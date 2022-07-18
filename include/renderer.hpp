#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "texture.hpp"
#include "uniform.hpp"
#include "constants.hpp"
#include "frame_data.hpp"
#include "material.hpp"

namespace gfx {

struct Window;
struct Device;
struct Mesh;

struct Renderer {
	VkFormat format;
	VkExtent2D extent;
	bool framebuffer_resized = false;
	VkSwapchainKHR swap_chain = VK_NULL_HANDLE;

	VkRenderPass render_pass = VK_NULL_HANDLE;
	Image depth_image;
	Image msaa_image;

	Uniform uniform;
	Texture texture;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	per_frame<VkDescriptorSet> descriptor_set;

	Frames frames;

	// These are all associated
	std::vector<VkImage> swap_chain_images;
	std::vector<VkImageView> swap_chain_image_views;
	std::vector<VkFramebuffer> framebuffers;

	void init(const Window& window, const Device& device);
	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr);
	void draw(Window& window, Device& device, Mesh& mesh, Material& material);

private:
	void init_swap_chain(const Device& device, const Window& window);
	void init_image_views(const VkDevice& device);
	void init_render_pass(const Device& device);
	void init_framebuffers(const VkDevice& device);
	void init_depth_image(const Device& device);
	void init_msaa_image(const Device& device);
	void init_base_descriptor(const Device& device);

	void record_command_buffer(
		VkCommandBuffer& command_buffer,
		const Mesh& mesh,
		const Material& material,
		int image_index);
	void recreate_swap_chain(Window&, Device&);
};

}
