#pragma once

#include <vulkan/vulkan_core.h>
#include <array>
#include "context.hpp"

namespace chch {

const int MAX_FRAMES_IN_FLIGHT = 2;

template <typename T>
using per_frame = std::array<T, MAX_FRAMES_IN_FLIGHT>;

struct FrameData {
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet descriptor_set;

	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
	VkFence in_flight_fence;

	VkResult init(const Context* context);
	void deinit(const Context* context);

private:
	VkResult init_command_buffer(const Context* context);
	VkResult init_sync_objects(const Context* context);
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

	void init(const Context* context) {
		for (auto& f : frame_data)
			f.init(context);
	}
	void deinit(const Context* context) {
		for (auto& f : frame_data)
			f.deinit(context);
	}
};

}
