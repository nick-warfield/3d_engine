#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <context.hpp>
#include "camera.hpp"
#include "frame_data.hpp"
#include "uniform.hpp"

#include <vector>

namespace chch {

struct Material;
struct Texture;

struct SceneGlobals {
	glm::vec3 sun_color;
	glm::vec3 sun_dir;
	float intensity;
	glm::vec3 ambient_color;
};

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
	per_frame<VkDescriptorSetLayout> descriptor_set_layout;
	per_frame<VkDescriptorSet> descriptor_set;
	Uniform<SceneGlobals> scene_uniform;

	Frames frames;
	Camera* camera;
	Context* context;

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

	void init(Context* p_context, SceneGlobals scene_globals, Camera* p_camera);
	void deinit();

	void setup_draw();
	void draw(const Transform& transform, const Mesh& mesh, const Material& material);
	void present_draw();

private:
	void init_swap_chain();
	void init_image_views();
	void init_render_pass();
	void init_framebuffers();
	void init_depth_image();
	void init_msaa_image();
	void init_base_descriptor();
	void init_camera();

	void record_command_buffer(
		VkCommandBuffer& command_buffer,
		const Transform& transform,
		const Mesh& mesh,
		const Material& material);
	void recreate_swap_chain();
};

}
