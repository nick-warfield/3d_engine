#pragma once

#include <glm/mat4x4.hpp>

namespace gfx {

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

}
