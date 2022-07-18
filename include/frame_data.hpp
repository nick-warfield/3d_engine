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

	void init(const Device& device);
	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr);

private:
	void init_command_buffer(const Device& device);
	void init_sync_objects(const Device& device);
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

	void init(const Device& device) {
		for (auto& f : frame_data)
			f.init(device);
	}
	void deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator = nullptr) {
		for (auto& f : frame_data)
			f.deinit(device, pAllocator);
	}
};

}
