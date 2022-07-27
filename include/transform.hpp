#pragma once

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/matrix.hpp"
#include "glm/gtc/quaternion.hpp"

namespace chch {

struct Transform {
	glm::vec3 position = glm::vec3(0.0f);
	glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	// +x = right
	// +y = up
	// right handed
	glm::mat4 matrix() const
	{
		return glm::translate(glm::mat4(1.0f), -position)
			* glm::mat4_cast(rotation)
			* glm::scale(glm::mat4(1.0f), scale);
	}
};

}
