#include "glm/ext/matrix_transform.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/trigonometric.hpp"

#include "buffer.hpp"
#include "constants.hpp"
#include "device.hpp"
#include "mesh.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "vertex.hpp"
#include "window.hpp"
#include "transform.hpp"

#include <array>
#include <chrono>
#include <vector>

static std::filesystem::path root_path;
const std::filesystem::path& gfx::Root::path(root_path);

using namespace gfx;

int main(int argc, char** argv)
{
	(void)argc;
	root_path = std::filesystem::absolute(argv[0]).parent_path();

	Window window;
	Device device;
	Renderer renderer;

	Mesh cube, sphere;
	Material material1, material2;

	Transform transform1 {
		glm::vec3(5.0f, 0.0f, 0.0f)
	};
	Transform transform2 {
		glm::vec3(-5.0f, -2.0f, 0.0f)
	};

	try {
		window.init("Vulkan Project", WIDTH, HEIGHT);
		device.init(window.instance, window.surface);
		renderer.init(window, device);

		sphere.init(device, "sphere.obj");
		cube.init(device, "cube.obj");

		material1.init(
			device,
			renderer.render_pass,
			renderer.descriptor_set_layout,
			"viking_room.png",
			"shader_vert.spv",
			"shader_frag.spv");

		material2.init(
			device,
			renderer.render_pass,
			renderer.descriptor_set_layout,
			"texture.jpg",
			"shader_vert.spv",
			"shader_frag.spv");

		static auto start_time = std::chrono::high_resolution_clock::now();

		// main loop
		while (!glfwWindowShouldClose(window.glfw_window)) {
			glfwPollEvents();
			auto current_time = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(
					current_time - start_time).count();

			transform2.rotation = glm::rotate(
					transform2.rotation,
					glm::radians(0.01f),
					glm::normalize(glm::vec3(1.0f, 1.3f, 0.4f)));

			transform1.scale = glm::vec3((1 + glm::sin(time)) / 2);

			renderer.setup_draw(window, device, material1.pipeline_layout);
			renderer.draw(transform1, sphere, material1);
			renderer.draw(transform2, cube, material2);
			renderer.present_draw(window, device);
		}
		vkDeviceWaitIdle(device.logical_device);

		// cleanup, reverse order of initiliztion
		material1.deinit(device.logical_device);
		material2.deinit(device.logical_device);
		sphere.deinit(device.logical_device);
		cube.deinit(device.logical_device);

		renderer.deinit(device.logical_device);
		device.deinit();
		window.deinit();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
