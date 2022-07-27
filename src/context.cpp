#include "context.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace chch {

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	(void)messageSeverity;
	(void)messageType;
	(void)pUserData;

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT& debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
	auto context = reinterpret_cast<Context*>(glfwGetWindowUserPointer(window));
	auto& capabilities = context->surface_capabilities;

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return;

	context->window_size.width = std::clamp(
		static_cast<uint32_t>(width),
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width);

	context->window_size.height = std::clamp(
		static_cast<uint32_t>(height),
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height);

	context->window_resized = true;
}

VkDebugUtilsMessengerCreateInfoEXT make_debugger_create_info()
{
	VkDebugUtilsMessengerCreateInfoEXT create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCallback;
	create_info.pUserData = nullptr; // Optional
	return create_info;
}

void Context::init(const ContextCreateInfo& create_info)
{
	enable_validation_layers = create_info.enable_validation_layers;
	validation_layers = create_info.validation_layers;
	required_extensions = create_info.required_extensions;
	preferred_extensions = create_info.preferred_extensions;
	found_extensions.resize(required_extensions.size() + preferred_extensions.size());
	allocation_callbacks = create_info.allocation_callbacks;

	init_glfw(create_info);
	init_instance(create_info);
	init_debugger();
	init_surface();
	init_physical_device();
	init_logical_device();
	init_queues();
	init_allocator();
	init_command_pool();
}

void Context::deinit()
{
	vkDestroyCommandPool(device, graphics_command_pool, allocation_callbacks);
	vkDestroyCommandPool(device, transfer_command_pool, allocation_callbacks);
	vmaDestroyAllocator(allocator);
	vkDestroyDevice(device, allocation_callbacks);
	vkDestroySurfaceKHR(instance, surface, allocation_callbacks);
	if (enable_validation_layers)
		DestroyDebugUtilsMessengerEXT(instance, debug_messenger, allocation_callbacks);
	vkDestroyInstance(instance, allocation_callbacks);
	glfwDestroyWindow(window);
	glfwTerminate();
}

uint32_t Context::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags property_flags) const
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

bool Context::window_hidden()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	return width == 0 || height == 0;
}
void Context::record_graphics_command(
		std::function<void(VkCommandBuffer command_buffer)> commands) const
{
	record_one_time_command(graphics_queue.queue, graphics_command_pool, commands);
}

void Context::record_transfer_command(
		std::function<void(VkCommandBuffer command_buffer)> commands) const
{
	record_one_time_command(transfer_queue.queue, transfer_command_pool, commands);
}

void Context::record_one_time_command(
		VkQueue queue,
		VkCommandPool command_pool,
		std::function<void(VkCommandBuffer command_buffer)> commands) const
{
	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

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

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

bool Context::supports_required_extensions()
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(
		physical_device,
		nullptr,
		&extension_count,
		available_extensions.data());

	for (const auto& req_ext : required_extensions) {
		bool extension_found = false;
		for (const auto& e : available_extensions) {
			extension_found = extension_found || (strcmp(e.extensionName, req_ext) == 0);
		}
		if (!extension_found) {
			return false;
		}
		found_extensions.push_back(req_ext);
	}
	return true;
}

void Context::populate_surface_info()
{
	if (!supports_required_extensions())
		return;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		physical_device,
		surface,
		&surface_capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
	if (format_count != 0) {
		supported_surface_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physical_device,
			surface,
			&format_count,
			supported_surface_formats.data());

		surface_format = supported_surface_formats[0];
		for (const auto& format : supported_surface_formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB
				&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surface_format = format;
			}
		}
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
	if (present_mode_count != 0) {
		supported_present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			physical_device,
			surface,
			&present_mode_count,
			supported_present_modes.data());

		present_mode = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& mode : supported_present_modes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				present_mode = mode;
		}
	}
}

void Context::populate_queue_family_indices()
{
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

	for (uint32_t i = 0; i < queue_family_count; ++i) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			graphics_queue.index = i;

		if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT
			&& !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			transfer_queue.index = i;

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
		if (present_support)
			present_queue.index = i;
	}

	if (!transfer_queue.is_available())
		transfer_queue.index = graphics_queue.index;

	// prefer graphics and present queue being the same queue
	VkBool32 present_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(
		physical_device,
		graphics_queue.index,
		surface,
		&present_support);
	if (present_support)
		present_queue.index = graphics_queue.index;
}

void Context::set_max_usable_sample_count()
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(physical_device, &props);

	VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts
		& props.limits.framebufferDepthSampleCounts;

	if (counts >= VK_SAMPLE_COUNT_64_BIT) {
		msaa_samples = VK_SAMPLE_COUNT_64_BIT;
	} else if (counts >= VK_SAMPLE_COUNT_32_BIT) {
		msaa_samples = VK_SAMPLE_COUNT_32_BIT;
	} else if (counts >= VK_SAMPLE_COUNT_16_BIT) {
		msaa_samples = VK_SAMPLE_COUNT_16_BIT;
	} else if (counts >= VK_SAMPLE_COUNT_8_BIT) {
		msaa_samples = VK_SAMPLE_COUNT_8_BIT;
	} else if (counts >= VK_SAMPLE_COUNT_4_BIT) {
		msaa_samples = VK_SAMPLE_COUNT_4_BIT;
	} else if (counts >= VK_SAMPLE_COUNT_2_BIT) {
		msaa_samples = VK_SAMPLE_COUNT_2_BIT;
	} else {
		msaa_samples = VK_SAMPLE_COUNT_1_BIT;
	}
}

void Context::populate_all_info()
{
	populate_surface_info();
	populate_queue_family_indices();
	set_max_usable_sample_count();
	vkGetPhysicalDeviceProperties(physical_device, &device_properties);
	vkGetPhysicalDeviceFeatures(physical_device, &device_features);
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
}

int Context::score_device()
{
	int score = 1000 * (int)(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
	score += device_properties.limits.maxImageDimension2D;

	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(
		physical_device,
		nullptr,
		&extension_count,
		available_extensions.data());

	for (const auto& pref_ext : preferred_extensions) {
		bool extension_found = false;
		for (const auto& e : available_extensions) {
			extension_found = extension_found || (strcmp(e.extensionName, pref_ext) == 0);
		}
		if (extension_found) {
			score += 1000;
			found_extensions.push_back(pref_ext);
		}
	}

	bool has_required_features = graphics_queue.is_available()
		&& present_queue.is_available()
		&& transfer_queue.is_available()
		&& !supported_surface_formats.empty()
		&& !supported_present_modes.empty()
		&& device_features.samplerAnisotropy
		&& device_features.sampleRateShading
		&& device_features.fillModeNonSolid
		&& device_features.geometryShader;

	return score * (int)has_required_features;
}

void Context::init_glfw(const ContextCreateInfo& create_info)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(
		create_info.window_size.width,
		create_info.window_size.height,
		create_info.app_name,
		nullptr,
		nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
}

void Context::init_instance(const ContextCreateInfo& create_info)
{
	// Validation layer support
	if (enable_validation_layers) {
		std::cout << "Requested Layers:\n";
		for (const auto& layer : validation_layers) {
			std::cout << '\t' << layer << '\n';
		}
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char* layer_name : validation_layers) {
			bool layer_found = false;
			for (const auto& layer_properties : available_layers) {
				layer_found = layer_found || (strcmp(layer_name, layer_properties.layerName) == 0);
			}
			if (!layer_found) {
				std::string msg = "validation layer ";
				msg += layer_name;
				msg += " not supported";
				throw std::runtime_error(msg);
			}
		}
	} // Validation Layer Support - End

	// Extension support
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	if (enable_validation_layers)
		glfw_extensions[glfw_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	std::cout << "Requested GLFW Extensions:\n";
	for (uint32_t i = 0; i < glfw_extension_count; ++i) {
		std::cout << '\t' << glfw_extensions[i] << '\n';
	}
	std::cout << '\n';

	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

	for (uint32_t i = 0; i < glfw_extension_count; ++i) {
		bool extension_found = false;
		for (const auto& e : available_extensions) {
			extension_found = extension_found || (strcmp(e.extensionName, glfw_extensions[i]) == 0);
		}
		if (!extension_found) {
			std::string msg = "required glfw extension ";
			msg += glfw_extensions[i];
			msg += " not found";
			throw std::runtime_error(msg);
		}
	}
	// Extension support - End

	// Create Instance
	VkApplicationInfo app_info {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = create_info.app_name;
	app_info.applicationVersion = create_info.app_version;
	app_info.pEngineName = "Choo Choo Engine";
	app_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instance_create_info {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &app_info;
	instance_create_info.enabledExtensionCount = glfw_extension_count;
	instance_create_info.ppEnabledExtensionNames = glfw_extensions;

	VkDebugUtilsMessengerCreateInfoEXT debugger_create_info {};
	if (enable_validation_layers) {
		instance_create_info.enabledLayerCount = static_cast<uint32_t>(create_info.validation_layers.size());
		instance_create_info.ppEnabledLayerNames = create_info.validation_layers.data();
		debugger_create_info = make_debugger_create_info();
		instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debugger_create_info);
	} else {
		instance_create_info.enabledLayerCount = 0;
		instance_create_info.pNext = nullptr;
	}

	if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance");
	} // Create Instance - End
}

void Context::init_debugger()
{
	if (!enable_validation_layers) {
		VkDebugUtilsMessengerCreateInfoEXT debugger_create_info = make_debugger_create_info();
		if (CreateDebugUtilsMessengerEXT(
				instance,
				&debugger_create_info,
				nullptr,
				&debug_messenger)
			!= VK_SUCCESS)
			throw std::runtime_error("failed to set up debug messenger");
	}
}

void Context::init_surface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface");
}

void Context::init_physical_device()
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0)
		throw std::runtime_error("failed to find GPU with Vulkan Support");

	std::vector<VkPhysicalDevice> physical_devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());
	std::map<int, Context> candidates;

	for (size_t i = 0; i < device_count; ++i) {
		Context dev;
		dev.physical_device = physical_devices[i];
		dev.surface = surface;
		dev.preferred_extensions = preferred_extensions;
		dev.required_extensions = required_extensions;
		dev.populate_all_info();
		candidates[dev.score_device()] = dev;
	}

	auto [best_score, best_device] = *candidates.rbegin();
	if (best_score == 0)
		throw std::runtime_error("failed to find suitable GPU");

	physical_device = best_device.physical_device;
	populate_all_info();
	found_extensions = best_device.found_extensions;
	std::cout << "Requested Device Extensions:\n";
	for (auto& ext : best_device.found_extensions)
		std::cout << '\t' << ext << '\n';
	std::cout << '\n';
}

void Context::init_logical_device()
{
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {
		graphics_queue.index,
		present_queue.index,
		transfer_queue.index
	};
	unique_queue_indices = std::vector<uint32_t>(
		unique_queue_families.begin(),
		unique_queue_families.end());

	float priority = 1.0f;
	for (uint32_t queue_family : unique_queue_indices) {
		VkDeviceQueueCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		create_info.queueFamilyIndex = queue_family;
		create_info.queueCount = 1;
		create_info.pQueuePriorities = &priority;

		queue_create_infos.push_back(create_info);
	}

	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(
		physical_device,
		nullptr,
		&extension_count,
		available_extensions.data());

	VkDeviceCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(found_extensions.size());
	create_info.ppEnabledExtensionNames = found_extensions.data();

	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");
}

void Context::init_queues()
{
	vkGetDeviceQueue(device, graphics_queue.index, 0, &graphics_queue.queue);
	vkGetDeviceQueue(device, present_queue.index, 0, &present_queue.queue);
	vkGetDeviceQueue(device, transfer_queue.index, 0, &transfer_queue.queue);
}

void Context::init_allocator()
{
	VmaAllocatorCreateInfo create_info {};
	create_info.instance = instance;
	create_info.device = device;
	create_info.physicalDevice = physical_device;
	create_info.pAllocationCallbacks = allocation_callbacks;
	create_info.vulkanApiVersion = VK_API_VERSION_1_3;

	vmaCreateAllocator(&create_info, &allocator);
}

void Context::init_command_pool()
{
	VkCommandPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	pool_info.queueFamilyIndex = transfer_queue.index;
	if (vkCreateCommandPool(device, &pool_info, nullptr, &transfer_command_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	pool_info.queueFamilyIndex = graphics_queue.index;
	if (vkCreateCommandPool(device, &pool_info, nullptr, &graphics_command_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");
}

}
