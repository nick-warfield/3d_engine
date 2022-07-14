
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include "glm/fwd.hpp"

#include "constants.hpp"
#include "window.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "buffer.hpp"
#include "vertex.hpp"
#include "texture.hpp"

#include <vector>
#include <array>

static std::filesystem::path root_path;
const std::filesystem::path& gfx::Root::path(root_path);

using namespace gfx;

int main(int argc, char** argv)
{
	(void)argc;
	root_path = std::filesystem::absolute(argv[0]).parent_path();

	Window window;
	Device device;
	BufferData buffers;
	Texture texture;
	Renderer renderer;

	try {
		window.init("Vulkan Project", WIDTH, HEIGHT);
		device.init(window.instance, window.surface);
		buffers.init(device);
		texture.init(device);
		renderer.init(window, device, buffers.uniform_buffers, texture);

		// main loop
		while (!glfwWindowShouldClose(window.glfw_window)) {
			glfwPollEvents();
			renderer.draw(window, device, buffers);
		}
		vkDeviceWaitIdle(device.logical_device);

		// cleanup, reverse order of initiliztion
		renderer.deinit(device);
		texture.deinit(device);
		buffers.deinit(device);
		device.deinit();
		window.deinit();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
