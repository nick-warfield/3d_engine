#pragma once

#include <vulkan/vulkan_core.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <array>
#include <functional>

namespace gfx {

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;

	bool operator==(const Vertex& other) const
	{
		return position == other.position
			&& normal == other.normal
			&& uv == other.uv;
	}

	static VkVertexInputBindingDescription get_binding_description()
	{
		VkVertexInputBindingDescription binding_description {};
		binding_description.binding = 0;
		binding_description.stride = sizeof(Vertex);
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return binding_description;
	}

	static std::array<VkVertexInputAttributeDescription, 3> get_attribute_description()
	{
		std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions {};

		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[0].offset = offsetof(Vertex, position);

		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, normal);

		attribute_descriptions[2].binding = 0;
		attribute_descriptions[2].location = 2;
		attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[2].offset = offsetof(Vertex, uv);

		return attribute_descriptions;
	}
};

}

template <>
struct std::hash<gfx::Vertex> {
	size_t operator()(gfx::Vertex const& vertex) const
	{
		return ((hash<glm::vec3>()(vertex.position)
				^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
				^ (hash<glm::vec2>()(vertex.uv) << 1);
	}
};
