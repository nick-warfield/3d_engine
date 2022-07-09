#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef DEBUG
const bool enable_validation_layers = true;
#else
const bool enable_validation_layers = false;
#endif

struct SwapChain {
	VkSwapchainKHR swap_chain;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
	VkFormat format;
	VkExtent2D extent;
};

const std::vector<const char*> validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	if (instance == VK_NULL_HANDLE) {
		throw std::runtime_error("HERE!");
	}
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

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

SwapChainSupportDetails querySwapChainSupport(
	const VkPhysicalDevice& device,
	const VkSurfaceKHR& surface)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
	if (format_count > 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device,
			surface,
			&format_count,
			details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
	if (present_mode_count > 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device,
			surface,
			&present_mode_count,
			details.present_modes.data());
	}

	return details;
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool has_all() const
	{
		return graphics_family.has_value()
			&& present_family.has_value();
	}
};

QueueFamilyIndices find_queue_families(const VkSurfaceKHR& surface, const VkPhysicalDevice& device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	for (uint32_t i = 0; i < queue_family_count; ++i) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphics_family = i;

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
		if (present_support)
			indices.present_family = i;
	}

	return indices;
}

void init_window(GLFWwindow*& window)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);
}

void init_instance(VkInstance& instance)
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
				char msg[128];
				strcat(msg, "validation layer ");
				strcat(msg, layer_name);
				strcat(msg, " not supported");
				throw std::runtime_error(msg);
			}
		}
	} // Validation Layer Support - End

	// Extension support
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	if (enable_validation_layers)
		glfwExtensions[glfwExtensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	std::cout << "Requested Extensions:\n";
	for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
		std::cout << '\t' << glfwExtensions[i] << '\n';
	}
	std::cout << '\n';

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
		bool extension_found = false;
		for (const auto& e : extensions) {
			extension_found = extension_found || (strcmp(e.extensionName, glfwExtensions[i]) == 0);
		}
		if (!extension_found) {
			char msg[128];
			strcat(msg, "required glfw extension ");
			strcat(msg, glfwExtensions[i]);
			strcat(msg, " not found");
			throw std::runtime_error(msg);
		}
	}
	// Extension support - End

	// Create Instance
	VkApplicationInfo appInfo {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
	if (enable_validation_layers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		createInfo.ppEnabledLayerNames = validation_layers.data();

		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		debugCreateInfo.pUserData = nullptr; // Optional

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debugCreateInfo);
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance");
	} // Create Instance - End
}

void init_debugger(const VkInstance& instance, VkDebugUtilsMessengerEXT& debug_messenger)
{
	if (!enable_validation_layers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger");
	}
}

void init_surface(const VkInstance& instance, GLFWwindow* window, VkSurfaceKHR& surface)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface");
	}
}

int rate_device(const VkSurfaceKHR& surface, const VkPhysicalDevice& device)
{
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);

	VkPhysicalDeviceFeatures dev_features;
	vkGetPhysicalDeviceFeatures(device, &dev_features);

	auto check_device_extension = [device]() {
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(
			device,
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
	};

	auto indices = find_queue_families(surface, device);
	auto swap_chain_details = check_device_extension()
		? querySwapChainSupport(device, surface)
		: SwapChainSupportDetails {};

	int score = 1000 * (int)(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
	score += device_properties.limits.maxImageDimension2D;

	bool has_required_features = indices.has_all()
		&& !swap_chain_details.formats.empty()
		&& !swap_chain_details.present_modes.empty()
		&& dev_features.geometryShader;

	return score * (int)has_required_features;
}

void pick_physical_device(
	const VkInstance& instance,
	VkSurfaceKHR& surface,
	VkPhysicalDevice& physical_device)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0)
		throw std::runtime_error("failed to find GPU with Vulkan Support");
	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	std::map<int, VkPhysicalDevice> candidates;
	for (const auto& dev : devices) {
		candidates[rate_device(surface, dev)] = dev;
	}
	auto best_device = *candidates.rbegin();
	if (best_device.first > 0)
		physical_device = best_device.second;

	if (physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find suitable GPU");
	}
}

void init_logical_device(
	const VkSurfaceKHR& surface,
	const VkPhysicalDevice& physical_device,
	VkDevice& device,
	VkQueue& graphics_queue,
	VkQueue& present_queue)
{
	QueueFamilyIndices indices = find_queue_families(surface, physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {
		indices.graphics_family.value(),
		indices.present_family.value(),
	};

	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		create_info.queueFamilyIndex = queue_family;
		create_info.queueCount = 1;
		float priority = 1.0f;
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

	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");

	// creating logical device also create queues for it automatically
	// only used one queue, so only need 0 for index
	vkGetDeviceQueue(device, *indices.graphics_family, 0, &graphics_queue);
	vkGetDeviceQueue(device, *indices.present_family, 0, &present_queue);
}

VkSurfaceFormatKHR choose_swap_chain_surface_format(
	const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	for (const auto& a_format : available_formats) {
		if (a_format.format == VK_FORMAT_B8G8R8A8_SRGB
			&& a_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return a_format;
		}
	}

	return available_formats[0];
}

VkPresentModeKHR choose_swap_chain_present_mode(
	const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const auto& a_mode : available_present_modes) {
		if (a_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return a_mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_chain_extent(
	GLFWwindow* window,
	const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	VkExtent2D extent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};
	extent.width = std::clamp(
		extent.width,
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width);
	extent.height = std::clamp(
		extent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height);
	return extent;
}

void init_swap_chain(
	GLFWwindow* window,
	const VkPhysicalDevice& physical_device,
	const VkDevice& device,
	const VkSurfaceKHR& surface,
	SwapChain& swap_chain)
{
	auto swap_chain_support = querySwapChainSupport(physical_device, surface);
	auto surface_format = choose_swap_chain_surface_format(swap_chain_support.formats);
	auto present_mode = choose_swap_chain_present_mode(swap_chain_support.present_modes);
	auto extent = choose_swap_chain_extent(window, swap_chain_support.capabilities);

	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0)
		image_count = std::min(image_count, swap_chain_support.capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = find_queue_families(surface, physical_device);
	uint32_t queue_family_indices[] = {
		indices.graphics_family.value(),
		indices.present_family.value()
	};

	if (indices.graphics_family != indices.present_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain.swap_chain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");

	vkGetSwapchainImagesKHR(device, swap_chain.swap_chain, &image_count, nullptr);
	swap_chain.images.resize(image_count);
	vkGetSwapchainImagesKHR(device, swap_chain.swap_chain, &image_count, swap_chain.images.data());
	swap_chain.format = surface_format.format;
	swap_chain.extent = extent;
}

void init_image_views(const VkDevice& device, SwapChain& swap_chain) {
	swap_chain.image_views.resize(swap_chain.images.size());
	for (size_t i = 0; i < swap_chain.images.size(); ++i) {
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = swap_chain.images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = swap_chain.format;

		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &create_info, nullptr, &swap_chain.image_views[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create image view");
	}
}

int main()
{
	GLFWwindow* window = nullptr;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
	VkPhysicalDevice physical_device;
	VkDevice device;

	VkSurfaceKHR surface;
	VkQueue graphics_queue;
	VkQueue present_queue;

	SwapChain swap_chain;

	try {
		init_window(window);
		init_instance(instance);
		init_debugger(instance, debug_messenger);
		init_surface(instance, window, surface);

		pick_physical_device(instance, surface, physical_device);
		init_logical_device(surface, physical_device, device, graphics_queue, present_queue);

		init_swap_chain(window, physical_device, device, surface, swap_chain);
		init_image_views(device, swap_chain);

		// main loop
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}

		// cleanup, reverse order of initiliztion
		for (auto view : swap_chain.image_views)
			vkDestroyImageView(device, view, nullptr);
		vkDestroySwapchainKHR(device, swap_chain.swap_chain, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		if (enable_validation_layers)
			DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);

		glfwTerminate();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
