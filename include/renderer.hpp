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
#include "window.hpp"
#include "device.hpp"

#include <vector>

namespace gfx {

static inline void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
	auto framebuffer_resized = reinterpret_cast<bool*>(glfwGetWindowUserPointer(window));
	*framebuffer_resized = true;
	(void)width;
	(void)height;
}

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

	template <typename T>
	void init(const Window& window, const Device& device, Camera* p_camera, T* ubo)
	{
		glfwSetWindowUserPointer(window.glfw_window, &framebuffer_resized);
		glfwSetFramebufferSizeCallback(window.glfw_window, framebuffer_resize_callback);

		init_swap_chain(device, window);

		auto dev = device.logical_device;
		init_image_views(dev);
		init_render_pass(device);
		init_depth_image(device);
		init_msaa_image(device);
		init_framebuffers(dev);

		texture.init(device, "viking_room.png");
		uniform.init(device, ubo);
		init_base_descriptor(device);

		camera = p_camera;
		frames.init(device);
	}

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
