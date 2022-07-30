#include "context.hpp"
#include "util.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "transform.hpp"

#include <filesystem>
#include <iostream>
#include <vulkan/vulkan_core.h>

static std::filesystem::path root_path;
const std::filesystem::path& chch::Root::path(root_path);

using namespace chch;

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

float lastX = 1920 / 2.0f, lastY = 1080 / 2.0f;
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

struct RenderObject {
	Transform transform;
	Mesh mesh;
	Material material;
};

struct FloorColor {
	glm::vec3 color;
};

struct SpecularData {
	glm::vec3 camera_direction;
	float width = 5.0f;
};

int main(int argc, char** argv)
{
	(void)argc;
	root_path = std::filesystem::absolute(argv[0]).parent_path();

	ContextCreateInfo cc_info {};
	cc_info.app_name = "test";
	cc_info.app_version = VK_MAKE_VERSION(0, 1, 0);
	cc_info.window_size = { 1920, 1080 };
	cc_info.enable_validation_layers = true;
	cc_info.validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};
	cc_info.required_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	cc_info.preferred_extensions = {
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
		VK_AMD_MEMORY_OVERALLOCATION_BEHAVIOR_EXTENSION_NAME
	};
	cc_info.allocation_callbacks = nullptr;

	Context context;
	Renderer renderer;

	Texture skyline, viking_room, statue;
	Uniform<FloorColor> floor_uniform;
	Uniform<SpecularData> spec_uniform;

	RenderObject sphere, cube, floor;
	RenderObject skybox;

	SceneGlobals globs;
	globs.sun_color = glm::vec3(1.0f, 1.0f, 1.0f);
	globs.sun_dir = glm::normalize(glm::vec3(60, 60, 60));
	globs.intensity = 0.5f;
	globs.ambient_color = glm::vec3(0.01f);

	try {
		context.init(cc_info);
		renderer.init(&context, globs, &camera);

		glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetKeyCallback(context.window, key_callback);
		glfwSetCursorPosCallback(context.window, cursor_position_callback);

		camera.width = context.surface_capabilities.currentExtent.width;
		camera.height = context.surface_capabilities.currentExtent.height;
		camera.fov = 55.0f;
		camera.depth_min = 0.1f;
		camera.depth_max = 100.0f;
		camera.type = Camera::PERSPECTIVE;
		camera.transform.position = glm::vec3(0.0f, 0.0f, -10.0f);

		skyline.init(&context, "skybox.png");
		viking_room.init(&context, "viking_room.png");
		statue.init(&context, "texture.jpg");

		floor_uniform.init(&context, { glm::vec3(0.3f) });
		spec_uniform.init(&context, { camera.transform.position, 0.5f });

		sphere.mesh.init(&context, "sphere.obj");
		sphere.material.init(&context,
				renderer.render_pass, renderer.descriptor_set_layout[0],
				{{ 1, &viking_room }},
				{{ 0, &spec_uniform.buffer }},
				"shader_vert.spv", "shader_frag.spv",
				VK_CULL_MODE_BACK_BIT, VK_TRUE);

		cube.mesh.init(&context, "cube.obj");
		cube.material.init(&context,
				renderer.render_pass, renderer.descriptor_set_layout[0],
				{{ 1, &statue }},
				{{ 0, &spec_uniform.buffer }},
				"shader_vert.spv", "shader_frag.spv",
				VK_CULL_MODE_BACK_BIT, VK_TRUE);

		floor.transform = Transform {
			glm::vec3(0.0f, -3.0f, 0.0f),
			glm::quat(0.0f, 0.0f, 0.0f, 1.0f),
			glm::vec3(20.0f, 1.0f, 20.0f)
		};
		floor.mesh.init(&context, "quad.obj");
		floor.material.init(&context,
				renderer.render_pass, renderer.descriptor_set_layout[0],
				{ },
				{{ 0, &floor_uniform.buffer }},
				"shader_vert.spv", "white_out_frag.spv",
				VK_CULL_MODE_BACK_BIT, VK_TRUE);

		skybox.mesh.init(&context, "skybox.obj");
		skybox.material.init(&context,
				renderer.render_pass, renderer.descriptor_set_layout[0],
				{{ 0, &skyline }},
				{ },
				"shader_vert.spv", "skybox_frag.spv",
				VK_CULL_MODE_FRONT_BIT, VK_FALSE);

		static auto start_time = std::chrono::high_resolution_clock::now();
		float last_time = 0.0f;

		while (!should_exit && !glfwWindowShouldClose(context.window)) {
			glfwPollEvents();

			auto current_time = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(
				current_time - start_time)
							 .count();
			float delta = last_time - time;

			auto v = glm::vec4(0.0f);
			if (forward)
				v.z -= 1.0f;
			if (backward)
				v.z += 1.0f;
			if (right)
				v.x += 1.0f;
			if (left)
				v.x -= 1.0f;
			glm::vec3 v2(camera.transform.matrix() * v * 50.0f * delta);
			camera.transform.position += v2;
			skybox.transform.position = camera.transform.position;

			spec_uniform.ubo().camera_direction = camera.transform.position;
			spec_uniform.update(renderer.frames.index);

			cube.transform.rotation = glm::rotate(
				cube.transform.rotation,
				glm::radians(0.01f),
				glm::normalize(glm::vec3(1.0f, 1.3f, 0.4f)));

			sphere.transform.position = glm::vec3(
					10 * (glm::sin(time)),
					0.0f,
					10 * (glm::cos(time)));

			renderer.setup_draw();
			renderer.draw(skybox.transform, skybox.mesh, skybox.material);

			renderer.draw(sphere.transform, sphere.mesh, sphere.material);
			renderer.draw(cube.transform, cube.mesh, cube.material);
			renderer.draw(floor.transform, floor.mesh, floor.material);

			renderer.present_draw();
			last_time = time;
		}
		vkDeviceWaitIdle(context.device);

		sphere.material.deinit(&context);
		sphere.mesh.deinit(&context);
		cube.material.deinit(&context);
		cube.mesh.deinit(&context);
		floor.material.deinit(&context);
		floor.mesh.deinit(&context);
		skybox.material.deinit(&context);
		skybox.mesh.deinit(&context);

		skyline.deinit(&context);
		viking_room.deinit(&context);
		statue.deinit(&context);

		floor_uniform.deinit(&context);
		spec_uniform.deinit(&context);

		renderer.deinit();
		context.deinit();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
