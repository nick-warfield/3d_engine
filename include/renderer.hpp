#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "texture.hpp"
#include "constants.hpp"

namespace gfx {

struct Window;
struct Device;
struct Buffer;
struct BufferData;
struct Texture;
struct Mesh;

struct Renderer {
	int width, height;
	int current_frame = 0;
	bool framebuffer_resized = false;

	Image depth_image;
	Image msaa_image;

	VkFormat format;
	VkExtent2D extent;
	VkSwapchainKHR swap_chain = VK_NULL_HANDLE;

	// I think i should move these to device and mirror the record_command() pattern
	// eg: Device::record_draw_commands(int current_frame, function<void(command_buffer)>)
	// I can use std::bind_right() to keep the draw commands in a function here
	VkCommandPool graphics_command_pool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> command_buffers;

	// These are all associated
	std::vector<VkImage> swap_chain_images;
	std::vector<VkImageView> swap_chain_image_views;
	std::vector<VkFramebuffer> framebuffers;

	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	per_frame<VkDescriptorSet> descriptor_sets;

	VkRenderPass render_pass = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	VkPipeline graphics_pipeline = VK_NULL_HANDLE;

	per_frame<VkSemaphore> image_available_semaphores;
	per_frame<VkSemaphore> render_finished_semaphores;
	per_frame<VkFence> in_flight_fences;

	void init(const Window& window,
		const Device& device,
		const per_frame<Buffer>& uniform_buffers,
		const Texture& texture);

	void deinit(const Device& device, const VkAllocationCallbacks* pAllocator = nullptr);
	void draw(Window& window, Device& device, Mesh& mesh);

private:
	void init_swap_chain(const Device& device, const Window& window);
	void init_image_views(const VkDevice& device);
	void init_render_pass(const Device& device);
	void init_descriptor_sets(const VkDevice& device,
		const per_frame<Buffer>& uniform_buffers,
		const Texture& texture);
	void init_graphics_pipeline(const Device& device);
	void init_framebuffers(const VkDevice& device);
	void init_command_buffers(const Device& device);
	void init_sync_objects(const VkDevice& device);
	void init_depth_image(const Device& device);
	void init_msaa_image(const Device& device);

	void record_command_buffer(
		VkCommandBuffer& command_buffer,
		const VkDescriptorSet& descriptor_set,
		const Mesh& mesh,
		int image_index);
	void recreate_swap_chain(Window&, Device&);
};

}
