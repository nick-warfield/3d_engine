#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef DEBUG
const bool enable_validation_layers = true;
#else
const bool enable_validation_layers = false;
#endif
const std::vector<const char*> validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

VkResult CreateDebugUtilsMessengerEXT(
	const VkInstance& instance,
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
	const VkInstance& instance,
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

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;

	bool has_all() const
	{
		return graphics_family.has_value();
	}
};

QueueFamilyIndices find_queue_families(const VkPhysicalDevice& device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	for (uint32_t i = 0; i < queue_family_count; ++i) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphics_family = i;
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

void init_vulkan(VkInstance& instance)
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

	if (enable_validation_layers) {
		glfwExtensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
		glfwExtensionCount++;
	}

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

void pick_physical_device(const VkInstance& instance, VkPhysicalDevice& physical_device)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0)
		throw std::runtime_error("failed to find GPU with Vulkan Support");
	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	auto rate_device = [](const VkPhysicalDevice& dev) {
		VkPhysicalDeviceProperties dev_properties;
		vkGetPhysicalDeviceProperties(dev, &dev_properties);

		VkPhysicalDeviceFeatures dev_features;
		vkGetPhysicalDeviceFeatures(dev, &dev_features);

		int score = 1000 * (int)(dev_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
		score += dev_properties.limits.maxImageDimension2D;

		// Required Features
		auto indices = find_queue_families(dev);
		score *= (int)indices.has_all();
		score *= (int)dev_features.geometryShader;

		return score;
	};

	std::map<int, VkPhysicalDevice> candidates;
	for (const auto& dev : devices) {
		candidates[rate_device(dev)] = dev;
	}
	auto best_device = *candidates.rbegin();
	if (best_device.first > 0)
		physical_device = best_device.second;

	if (physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find suitable GPU");
	}
}

void create_logical_device(
		const VkPhysicalDevice& physical_device,
		VkDevice& device,
		VkQueue& graphics_queue)
{
	QueueFamilyIndices indices = find_queue_families(physical_device);

	VkDeviceQueueCreateInfo queue_create_info {};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = indices.graphics_family.value();
	queue_create_info.queueCount = 1;
	float priority = 1.0f;
	queue_create_info.pQueuePriorities = &priority;

	VkPhysicalDeviceFeatures device_features {};
	// todo: finish this

	VkDeviceCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = &queue_create_info;
	create_info.queueCreateInfoCount = 1;
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = 0;

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
}

int main()
{
	GLFWwindow* window = nullptr;
	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphics_queue;

	try {
		init_window(window);
		init_vulkan(instance);
		init_debugger(instance, debug_messenger);

		pick_physical_device(instance, physical_device);
		create_logical_device(physical_device, device, graphics_queue);

		// main loop
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}

		// cleanup, reverse order of initiliztion
		vkDestroyDevice(device, nullptr);
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
