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
    {{-0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}, { 1.0f, 0.0f }},
    {{ 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, { 0.0f, 0.0f }},
    {{ 0.5f,  0.5f}, {0.0f, 0.0f, 0.0f}, { 0.0f, 1.0f }},
    {{-0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, { 1.0f, 1.0f }}
};
const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
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
