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
			// not working for some reason
			cached_projection = glm::ortho(0.0f, width, height, 0.0f);
			break;
		}

		// glm adjustment for vulkan coordinate system
		cached_projection[1][1] *= -1;
		cache_good = true;
		return cached_projection * glm::inverse(transform.matrix());
	}
};

}
