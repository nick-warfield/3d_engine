#pragma once

#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <vector>
#include <filesystem>

#include "vertex.hpp"

namespace gfx {

struct Root {
	static const std::filesystem::path& path;
};

const std::vector<Vertex> vertices = {
	{{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
	{{ 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
	{{ 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},

	{{ 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
	{{ 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
	{{ 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }},
};
const std::vector<uint16_t> indices = {
	0, 1, 5, 5, 3, 0,	// front
	4, 2, 6, 6, 7, 4,	// back
	0, 2, 4, 4, 1, 0,	// bottom
	3, 5, 7, 7, 6, 3,	// top
	0, 3, 6, 6, 2, 0,	// left side
	1, 4, 7, 7, 5, 1	// right side
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
