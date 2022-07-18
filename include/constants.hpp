#pragma once

#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <vector>
#include <array>
#include <filesystem>
#include <string>

#include "vertex.hpp"

namespace gfx {

struct Root {
	static const std::filesystem::path& path;
};

const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;

const std::string MODEL_PATH = "viking_room.obj";
const std::string TEXTURE_PATH = "viking_room.png";

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
