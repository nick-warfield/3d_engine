#include "constants.hpp"
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.hpp"
#include "device.hpp"
#include "renderer.hpp"

using namespace gfx;

static std::filesystem::path root_path;
const std::filesystem::path& Root::path(root_path);

int main(int argc, char** argv)
{
	(void)argc;
	root_path = std::filesystem::absolute(argv[0]).parent_path();

	Window window;
	Device device;
	Renderer renderer;

	try {
		window.init("Vulkan Project", WIDTH, HEIGHT);
		device.init(window.instance, window.surface);
		renderer.init(window, device);

		// main loop
		while (!glfwWindowShouldClose(window.glfw_window)) {
			glfwPollEvents();
			renderer.draw(window, device);
		}
		vkDeviceWaitIdle(device.logical_device);

		// cleanup, reverse order of initiliztion
		renderer.deinit(device);
		device.deinit();
		window.deinit();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
