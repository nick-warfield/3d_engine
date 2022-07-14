#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace gfx {

struct Window;
struct Device;
struct Buffer;
struct BufferData;

struct Renderer {
	int width, height;
	int current_frame = 0;
	bool framebuffer_resized = false;

	VkFormat format;
	VkExtent2D extent;
	VkSwapchainKHR swap_chain = VK_NULL_HANDLE;

	VkCommandPool graphics_command_pool = VK_NULL_HANDLE;
	VkCommandPool transfer_command_pool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> command_buffers;

	// These are all associated
	std::vector<VkImage> swap_chain_images;
	std::vector<VkImageView> swap_chain_image_views;
	std::vector<VkFramebuffer> framebuffers;

	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptor_sets;

	VkRenderPass render_pass = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	VkPipeline graphics_pipeline = VK_NULL_HANDLE;

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;

	void init(const Window& window,
		const Device& device,
		const std::vector<Buffer>& uniform_buffers);

	void deinit(const Device& device, const VkAllocationCallbacks* pAllocator = nullptr);
	void draw(Window& window, Device& device, BufferData& buffers);

private:
	void init_swap_chain(const Device& device, const Window& window);
	void init_image_views(const VkDevice& device);
	void init_render_pass(const VkDevice& device);
	void init_descriptor_sets(const VkDevice& device,
		const std::vector<Buffer>& uniform_buffers);
	void init_graphics_pipeline(const VkDevice& device);
	void init_framebuffers(const VkDevice& device);
	void init_command_buffers(const Device& device);
	void init_sync_objects(const VkDevice& device);

	void record_command_buffer(
		VkCommandBuffer& command_buffer,
		const VkDescriptorSet& descriptor_set,
		const VkBuffer& vertex_buffer,
		const VkBuffer& index_buffer,
		int image_index);
	void recreate_swap_chain(Window&, Device&);
};

}
