#include "window.hpp"

#include "constants.hpp"
#include <cstring>
#include <iostream>

namespace gfx {

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

void Window::init(const char* name, int width, int height)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfw_window = glfwCreateWindow(width, height, name, nullptr, nullptr);

	// Move this into Renderer.init()
	// glfwSetWindowUserPointer(glfw_window, framebuffer_resized);
	// glfwSetFramebufferSizeCallback(glfw_window, framebuffer_resize_callback);

	init_instance(name);

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

	if (glfwCreateWindowSurface(instance, glfw_window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface");
}

void Window::deinit(const VkAllocationCallbacks* pAllocator) {
	vkDestroySurfaceKHR(instance, surface, pAllocator);
	if (enable_validation_layers)
		DestroyDebugUtilsMessengerEXT(instance, debug_messenger, pAllocator);
	vkDestroyInstance(instance, pAllocator);
	glfwDestroyWindow(glfw_window);

	glfwTerminate();
}

void Window::init_instance(const char* name)
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
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	if (enable_validation_layers)
		glfw_extensions[glfw_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	std::cout << "Requested Extensions:\n";
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
			char msg[128];
			strcat(msg, "required glfw extension ");
			strcat(msg, glfw_extensions[i]);
			strcat(msg, " not found");
			throw std::runtime_error(msg);
		}
	}
	// Extension support - End

	// Create Instance
	VkApplicationInfo app_info {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = name;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "3D Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = glfw_extension_count;
	create_info.ppEnabledExtensionNames = glfw_extensions;

	VkDebugUtilsMessengerCreateInfoEXT debugger_create_info {};
	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
		debugger_create_info = make_debugger_create_info();
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debugger_create_info);
	} else {
		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;
	}

	if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance");
	} // Create Instance - End
}

};
