#include "device.hpp"
#include "constants.hpp"
#include "window.hpp"

#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace gfx {

void Device::init(const VkInstance& instance, const VkSurfaceKHR& surface)
{
	// Find Best Physical Device
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0)
		throw std::runtime_error("failed to find GPU with Vulkan Support");

	std::vector<VkPhysicalDevice> physical_devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());
	std::map<int, Device> candidates;

	for (size_t i = 0; i < device_count; ++i) {
		Device dev;
		dev.physical_device = physical_devices[i];
		dev.populate_swap_chain_support_info(surface);
		dev.populate_queue_family_indices(surface);
		dev.set_max_usable_sample_count();
		candidates[dev.score_device()] = dev;
	}

	auto [best_score, best_device] = *candidates.rbegin();
	if (best_score == 0)
		throw std::runtime_error("failed to find suitable GPU");
	// Find Best Physical Device - End

	physical_device = best_device.physical_device;
	supported_swap_chain_features = best_device.supported_swap_chain_features;
	graphics_queue_family = best_device.graphics_queue_family;
	present_queue_family = best_device.present_queue_family;
	transfer_queue_family = best_device.transfer_queue_family;
	msaa_samples = best_device.msaa_samples;
	init_logical_device();

	vkGetDeviceQueue(logical_device,
		graphics_queue_family.index.value(),
		0,
		&graphics_queue_family.queue);

	vkGetDeviceQueue(
		logical_device,
		present_queue_family.index.value(),
		0,
		&present_queue_family.queue);

	vkGetDeviceQueue(
		logical_device,
		transfer_queue_family.index.value(),
		0,
		&transfer_queue_family.queue);

	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	VkCommandPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	pool_info.queueFamilyIndex = transfer_queue_family.index.value();
	if (vkCreateCommandPool(logical_device, &pool_info, nullptr, &temp_command_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	pool_info.queueFamilyIndex = graphics_queue_family.index.value();
	if (vkCreateCommandPool(logical_device, &pool_info, nullptr, &graphics_command_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");
}

void Device::deinit(const VkAllocationCallbacks* pAllocator)
{
	vkDestroyCommandPool(logical_device, temp_command_pool, pAllocator);
	vkDestroyCommandPool(logical_device, graphics_command_pool, pAllocator);
	vkDestroyDevice(logical_device, pAllocator);
}

void Device::update_swap_chain_support_info(const Window& window)
{
	populate_swap_chain_support_info(window.surface);
}

bool Device::supports_required_extensions()
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(
		physical_device,
		nullptr,
		&extension_count,
		available_extensions.data());

	for (const auto& req_ext : device_extensions) {
		bool extension_found = false;
		for (const auto& e : available_extensions) {
			extension_found = extension_found || (strcmp(e.extensionName, req_ext) == 0);
		}
		if (!extension_found) {
			return false;
		}
	}
	return true;
}

std::vector<uint32_t> Device::get_unique_queue_family_indices() const
{
	std::set<uint32_t> unique_queue_families = {
		graphics_queue_family.index.value(),
		present_queue_family.index.value(),
		transfer_queue_family.index.value()
	};

	return std::vector<uint32_t>(unique_queue_families.begin(), unique_queue_families.end());
}

void Device::populate_swap_chain_support_info(
	const VkSurfaceKHR& surface)
{
	if (!supports_required_extensions())
		return;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		physical_device,
		surface,
		&supported_swap_chain_features.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
	if (format_count != 0) {
		supported_swap_chain_features.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physical_device,
			surface,
			&format_count,
			supported_swap_chain_features.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
	if (present_mode_count != 0) {
		supported_swap_chain_features.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			physical_device,
			surface,
			&present_mode_count,
			supported_swap_chain_features.present_modes.data());
	}
}

void Device::populate_queue_family_indices(
	const VkSurfaceKHR& surface)
{
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

	for (uint32_t i = 0; i < queue_family_count; ++i) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			graphics_queue_family.index = i;

		if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT
			&& !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			transfer_queue_family.index = i;

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
		if (present_support)
			present_queue_family.index = i;
	}

	if (!transfer_queue_family.index.has_value())
		transfer_queue_family.index = graphics_queue_family.index;

	// prefer graphics and present queue being the same queue
	VkBool32 present_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(
			physical_device,
			graphics_queue_family.index.value(),
			surface,
			&present_support);
	if (present_support)
		present_queue_family.index = graphics_queue_family.index.value();
}

int Device::score_device()
{
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &device_properties);

	VkPhysicalDeviceFeatures dev_features;
	vkGetPhysicalDeviceFeatures(physical_device, &dev_features);

	int score = 1000 * (int)(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
	score += device_properties.limits.maxImageDimension2D;

	bool has_required_features = graphics_queue_family.index.has_value()
		&& present_queue_family.index.has_value()
		&& transfer_queue_family.index.has_value()
		&& dev_features.samplerAnisotropy
		&& !supported_swap_chain_features.formats.empty()
		&& !supported_swap_chain_features.present_modes.empty()
		&& dev_features.geometryShader;

	return score * (int)has_required_features;
}

void Device::init_logical_device()
{
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {
		graphics_queue_family.index.value(),
		present_queue_family.index.value(),
		transfer_queue_family.index.value()
	};

	float priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		create_info.queueFamilyIndex = queue_family;
		create_info.queueCount = 1;
		create_info.pQueuePriorities = &priority;

		queue_create_infos.push_back(create_info);
	}

	VkPhysicalDeviceFeatures device_features {};
	device_features.samplerAnisotropy = VK_TRUE;
	device_features.sampleRateShading = VK_TRUE;
	device_features.fillModeNonSolid = VK_TRUE;

	VkDeviceCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &create_info, nullptr, &logical_device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");
}

uint32_t Device::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags property_flags) const
{
	auto mem_props = memory_properties;
	for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
		auto flags = mem_props.memoryTypes[i].propertyFlags;
		if (type_filter & (1 << i)
			&& (flags & property_flags) == property_flags)
			return i;
	}
	throw std::runtime_error("failed to find suitable memory type");
}

void Device::record_graphics_commands(std::function<void(VkCommandBuffer command_buffer)> commands) const
{
	record_commands(graphics_command_pool, graphics_queue_family.queue, commands);
}

void Device::record_transfer_commands(std::function<void(VkCommandBuffer command_buffer)> commands) const
{
	record_commands(temp_command_pool, transfer_queue_family.queue, commands);
}

void Device::record_commands(
		VkCommandPool command_pool,
		VkQueue queue,
		std::function<void(VkCommandBuffer command_buffer)> commands) const
{
	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(logical_device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);
	commands(command_buffer);
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buffer);
}

void Device::set_max_usable_sample_count() {
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(physical_device, &props);

	VkSampleCountFlags counts =
		props.limits.framebufferColorSampleCounts
		& props.limits.framebufferDepthSampleCounts;

	if (counts >= VK_SAMPLE_COUNT_64_BIT) { msaa_samples = VK_SAMPLE_COUNT_64_BIT; }
	else if (counts >= VK_SAMPLE_COUNT_32_BIT) { msaa_samples = VK_SAMPLE_COUNT_32_BIT; }
	else if (counts >= VK_SAMPLE_COUNT_16_BIT) { msaa_samples = VK_SAMPLE_COUNT_16_BIT; }
	else if (counts >= VK_SAMPLE_COUNT_8_BIT ) { msaa_samples = VK_SAMPLE_COUNT_8_BIT;  }
	else if (counts >= VK_SAMPLE_COUNT_4_BIT ) { msaa_samples = VK_SAMPLE_COUNT_4_BIT;  }
	else if (counts >= VK_SAMPLE_COUNT_2_BIT ) { msaa_samples = VK_SAMPLE_COUNT_2_BIT;  }
	else { msaa_samples = VK_SAMPLE_COUNT_1_BIT; }
}

}
