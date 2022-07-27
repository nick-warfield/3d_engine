#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <context.hpp>
#include "camera.hpp"
#include "frame_data.hpp"
#include "texture.hpp"
#include "uniform.hpp"

#include <vector>

namespace chch {

struct Material;

struct SceneGlobals {

};

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
	VkSwapchainKHR swap_chain = VK_NULL_HANDLE;

	VkRenderPass render_pass = VK_NULL_HANDLE;
	Image depth_image;
	Image msaa_image;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	per_frame<VkDescriptorSet> descriptor_set;
	Uniform<SceneGlobals> scene_globals;

	Frames frames;
	Camera* camera;
	const Context* context;

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

	void init(const Context* p_context, Camera* p_camera)
	{
		context = p_context;
		init_swap_chain();

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
