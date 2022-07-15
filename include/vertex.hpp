#pragma once

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include <array>

namespace gfx {

struct Vertex {
	glm::vec3 position;
	glm::vec2 tex_coord;

	static VkVertexInputBindingDescription get_binding_description() {
        VkVertexInputBindingDescription binding_description{};
		binding_description.binding = 0;
		binding_description.stride = sizeof(Vertex);
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

	static std::array<VkVertexInputAttributeDescription, 2> get_attribute_description() {
		std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions {};

		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[0].offset = offsetof(Vertex, position);

		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, tex_coord);

		return attribute_descriptions;
	}
};

}
