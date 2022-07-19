#pragma once

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/matrix.hpp"

#include "transform.hpp"

#include <iostream>

namespace gfx {

struct Camera {
	Transform transform;

	bool cache_good = false;
	glm::mat4 cached_projection = glm::mat4(1.0f);

	float width, height;
	float depth_min, depth_max;
	float fov;

	enum {
		PERSPECTIVE,
		ORTHOGRAPHIC
	} type;

	glm::mat4 matrix()
	{
		if (cache_good)
			return cached_projection * glm::inverse(transform.matrix());

		switch (type) {
		case PERSPECTIVE:
			cached_projection = glm::perspective(glm::radians(fov), width / height, depth_min, depth_max);
			break;
		case ORTHOGRAPHIC:
			cached_projection = glm::ortho(
					-width / 2.0f, width / 2.0f,
					-height / 2.0f, height / 2.0f,
					depth_max, depth_min);
			break;
		}

		// glm adjustment for vulkan coordinate system
		//cached_projection[1][1] *= -1;
		cache_good = true;
		return cached_projection * glm::inverse(transform.matrix());
	}
};

}
