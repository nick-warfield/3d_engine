#include "device.hpp"
#include "window.hpp"
#include "constants.hpp"

#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

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
	init_logical_device();

	// creating logical device also create queues for it automatically
	// only used one queue, so only need 0 for index
	vkGetDeviceQueue(
		logical_device,
		graphics_queue_family.index.value(),
		0,
		&graphics_queue_family.queue);
	vkGetDeviceQueue(
		logical_device,
		present_queue_family.index.value(),
		0,
		&present_queue_family.queue);
}

void Device::deinit(const VkAllocationCallbacks* pAllocator)
{
	vkDestroyDevice(logical_device, pAllocator);
}

void Device::update_swap_chain_support_info(const Window& window) {
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

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
		if (present_support)
			present_queue_family.index = i;
	}
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
	// todo: finish this

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

}
