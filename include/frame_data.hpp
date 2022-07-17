#pragma once

#include <vulkan/vulkan_core.h>

#include <array>
#include "buffer.hpp"

namespace gfx {

const int MAX_FRAMES_IN_FLIGHT = 2;

template <typename T>
using per_frame = std::array<T, MAX_FRAMES_IN_FLIGHT>;

struct FrameData {
	// Flush pool every frame
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
	VkFence in_flight_fence;
};

struct Frames {
	per_frame<FrameData> frame_data;
	int index = 0;

	void next() {
		index = (index + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	FrameData& current_frame() {
		return frame_data[index];
	}
};

}
