#include "glm/ext/matrix_transform.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
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

	UniformBufferObject transform1 {
		glm::translate(
			glm::mat4(1.0f),
			glm::vec3(0.0f, 5.0f, 0.0f)),
		glm::rotate(
			glm::mat4(1.0),
			glm::radians(0.0f),
			glm::vec3(0.0, 0.0, 1.0)),
		glm::scale(
			glm::mat4(1.0f),
			glm::vec3(1.0f))
	};

	UniformBufferObject transform2 {
		glm::translate(
			glm::mat4(1.0f),
			glm::vec3(0.0f, -2.0f, 2.0f)),
		glm::rotate(
			glm::mat4(1.0),
			glm::radians(0.0f),
			glm::vec3(0.0, 0.0, 1.0)),
		glm::scale(
			glm::mat4(1.0f),
			glm::vec3(1.0f))
	};

	try {
		window.init("Vulkan Project", WIDTH, HEIGHT);
		device.init(window.instance, window.surface);
		renderer.init(window, device);

		UniformBufferObject camera {
			glm::rotate(
				glm::mat4(1.0f),
				glm::radians(0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::lookAt(
				glm::vec3(10.0f, 2.0f, 2.0f),
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::perspective(
				glm::radians(65.0f),
				((float)renderer.extent.width / (float)renderer.extent.height),
				0.1f,
				100.0f)
		};
		camera.projection[1][1] *= -1;

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

		renderer.uniform.update(device.logical_device, camera, renderer.frames.index);
		renderer.uniform.update(device.logical_device, camera, renderer.frames.index + 1);

		material1.uniform.update(device.logical_device, transform1, renderer.frames.index);
		material1.uniform.update(device.logical_device, transform1, renderer.frames.index + 1);

		material2.uniform.update(device.logical_device, transform2, renderer.frames.index);
		material2.uniform.update(device.logical_device, transform2, renderer.frames.index + 1);

		static auto start_time = std::chrono::high_resolution_clock::now();

		// main loop
		while (!glfwWindowShouldClose(window.glfw_window)) {
			glfwPollEvents();
			auto current_time = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(
					current_time - start_time).count();

			auto ubo = material1.uniform.ubo;
			ubo.projection = glm::scale(glm::mat4(1.0f), glm::vec3((1 + glm::sin(time)) / 2));
			material1.uniform.update(device.logical_device, ubo, renderer.frames.index);

			ubo = material2.uniform.ubo;
			ubo.view = glm::rotate(
					ubo.view,
					glm::radians(0.01f),
					glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)));
			material2.uniform.update(device.logical_device, ubo, renderer.frames.index);

			renderer.setup_draw(window, device);
			renderer.draw(sphere, material1);
			renderer.draw(cube, material2);
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
