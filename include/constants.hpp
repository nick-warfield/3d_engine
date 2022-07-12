#pragma once

#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <vector>
#include <filesystem>

namespace gfx {

struct Root {
	static const std::filesystem::path& path;
};

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef DEBUG
const bool enable_validation_layers = true;
#else
const bool enable_validation_layers = false;
#endif

const std::vector<const char*> validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

}
