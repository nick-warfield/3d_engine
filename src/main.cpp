#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_transform.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/trigonometric.hpp"

#include "buffer.hpp"
#include "constants.hpp"
#include "device.hpp"
#include "mesh.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "transform.hpp"
#include "vertex.hpp"
#include "window.hpp"

#include <array>
#include <chrono>
#include <vector>

static std::filesystem::path root_path;
const std::filesystem::path& gfx::Root::path(root_path);

using namespace gfx;

bool should_exit = false;
Camera camera;

bool left = false, right = false, forward = false, backward = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)window;
	(void)scancode;
	(void)mods;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		should_exit = true;

	if (key == GLFW_KEY_W)
		forward = action != GLFW_RELEASE ? true : false;
	if (key == GLFW_KEY_S)
		backward = action != GLFW_RELEASE ? true : false;
	if (key == GLFW_KEY_D)
		right = action != GLFW_RELEASE ? true : false;
	if (key == GLFW_KEY_A)
		left = action != GLFW_RELEASE ? true : false;
}

float lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f;
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	(void)window;
	float xoffset = lastX - xpos;
	float yoffset = lastY - ypos;
	xoffset *= 0.05f;
	yoffset *= 0.05f;

	camera.transform.rotation = glm::rotate(
		camera.transform.rotation,
		glm::radians(xoffset),
		glm::vec3(0.0f, 1.0f, 0.0f));
	camera.transform.rotation = glm::rotate(
		camera.transform.rotation,
		glm::radians(yoffset),
		glm::vec3(1.0f, 0.0f, 0.0f));

	lastX = xpos;
	lastY = ypos;
}

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
		renderer.init(window, device, &camera);

		glfwSetInputMode(window.glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetKeyCallback(window.glfw_window, key_callback);
		glfwSetCursorPosCallback(window.glfw_window, cursor_position_callback);

		camera.width = renderer.extent.width;
		camera.height = renderer.extent.height;
		camera.fov = 65.0f;
		camera.depth_min = 0.1f;
		camera.depth_max = 100.0f;
		camera.type = Camera::PERSPECTIVE;

		camera.transform.position = glm::vec3(0.0f, -6.0f, 15.0f);
		camera.transform.rotation = glm::rotate(
			camera.transform.rotation,
			glm::radians(-20.0f),
			glm::vec3(1.0f, 0.0f, 0.0f));

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
		float last_time = 0.0f;

		// main loop
		while (!should_exit && !glfwWindowShouldClose(window.glfw_window)) {
			glfwPollEvents();

			auto current_time = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(
				current_time - start_time)
							 .count();
			float delta = last_time - time;

			transform2.rotation = glm::rotate(
				transform2.rotation,
				glm::radians(0.01f),
				glm::normalize(glm::vec3(1.0f, 1.3f, 0.4f)));

			transform1.scale = glm::vec3((1 + glm::sin(time)) / 2);

			auto v = glm::vec4(0.0f);
			if (forward)
				v.z += 1.0f;
			if (backward)
				v.z -= 1.0f;
			if (right)
				v.x -= 1.0f;
			if (left)
				v.x += 1.0f;
			glm::vec3 v2(camera.transform.matrix() * v * 50.0f * delta);
			camera.transform.position += v2;

			renderer.setup_draw(window, device, material1.pipeline_layout);
			renderer.draw(transform1, sphere, material1);
			renderer.draw(transform2, cube, material2);
			renderer.present_draw(window, device);

			last_time = time;
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
