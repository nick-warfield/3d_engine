#pragma once

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/matrix.hpp"

#include "transform.hpp"

namespace gfx {

struct Camera {
	Transform transform;

	float width, height;
	float depth_min, depth_max;
	float fov;

	enum {
		PERSPECTIVE,
		ORTHOGRAPHIC
	} type;

	glm::mat4 matrix() const
	{
		glm::mat4 projection;

		switch (type) {
		case PERSPECTIVE:
			projection = glm::perspective(glm::radians(fov), width / height, depth_min, depth_max);
			break;
		case ORTHOGRAPHIC:
			projection = glm::ortho(0.0f, width, height, 0.0f, depth_min, depth_max);
			break;
		}

		// glm adjustment for vulkan coordinate system
		projection[1][1] *= -1;
		return projection * glm::inverse(transform.matrix());
	}
};

}
