#include "constants.hpp"
#include "context.hpp"
#include <filesystem>
#include <iostream>
#include <vulkan/vulkan_core.h>

static std::filesystem::path root_path;
const std::filesystem::path& gfx::Root::path(root_path);

using namespace chch;

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

	auto should_exit = false;
	Context context;

	try {
		context.init(cc_info);

		while (!should_exit && !glfwWindowShouldClose(context.window)) {
			glfwPollEvents();
		}

		context.deinit();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
