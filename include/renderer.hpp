#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "camera.hpp"
#include "constants.hpp"
#include "frame_data.hpp"
#include "material.hpp"
#include "texture.hpp"
#include "uniform.hpp"
#include <vector>

namespace gfx {

struct Window;
struct Device;
struct Mesh;

struct Renderer {
	uint32_t image_index;
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
	Camera* camera;

	const glm::mat4 correction_matrix = {
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f,-1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.5f, 0.5f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};

	// These are all associated
	std::vector<VkImage> swap_chain_images;
	std::vector<VkImageView> swap_chain_image_views;
	std::vector<VkFramebuffer> framebuffers;

	void init(const Window& window, const Device& device, Camera* p_camera);
	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr);

	void setup_draw(Window& window, Device& device, VkPipelineLayout pipeline_layout);
	void draw(const Transform& transform, const Mesh& mesh, const Material& material);
	void present_draw(Window& window, Device& device);

private:
	void init_swap_chain(const Device& device, const Window& window);
	void init_image_views(const VkDevice& device);
	void init_render_pass(const Device& device);
	void init_framebuffers(const VkDevice& device);
	void init_depth_image(const Device& device);
	void init_msaa_image(const Device& device);
	void init_base_descriptor(const Device& device);
	void init_camera();

	void record_command_buffer(
		VkCommandBuffer& command_buffer,
		const Transform& transform,
		const Mesh& mesh,
		const Material& material);
	void recreate_swap_chain(Window&, Device&);
};

}
