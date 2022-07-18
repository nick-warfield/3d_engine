
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
#include "mesh.hpp"

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
	Renderer renderer;

	Mesh mesh;
	//Texture texture;
	//Uniform uniform;
	//Material material;

	try {
		window.init("Vulkan Project", WIDTH, HEIGHT);
		device.init(window.instance, window.surface);
		renderer.init(window, device);

		mesh.init(device);
		//uniform.init(device);
		//texture.init(device);
		//material.init(device);

		// main loop
		while (!glfwWindowShouldClose(window.glfw_window)) {
			glfwPollEvents();
			renderer.base_material.uniform.update(
					device.logical_device,
					renderer.extent,
					renderer.frames.index);
			renderer.draw(window, device, mesh, renderer.base_material);
		}
		vkDeviceWaitIdle(device.logical_device);

		// cleanup, reverse order of initiliztion
		//texture.deinit(device.logical_device);
		//uniform.deinit(device.logical_device);
		mesh.deinit(device.logical_device);

		renderer.deinit(device.logical_device);
		device.deinit();
		window.deinit();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
