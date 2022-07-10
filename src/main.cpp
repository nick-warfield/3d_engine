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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<char> read_file(
	const std::filesystem::path root,
	const std::string& filename)
{
	std::ifstream file((root / filename).string(), std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("failed to open file");

	size_t file_size = (size_t)file.tellg();
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

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

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

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
struct SwapChain {
	VkSwapchainKHR swap_chain;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
	VkFormat format;
	VkExtent2D extent;

	SwapChainSupportDetails support_details;
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

struct QueueFamily {
	std::optional<uint32_t> index;
	VkQueue queue = VK_NULL_HANDLE;
};
std::vector<QueueFamily> find_queue_families(
	const VkSurfaceKHR& surface,
	const VkPhysicalDevice& device);

struct Device {
	VkPhysicalDevice physical_device;
	VkDevice logical_device;

	enum {
		GRAPHICS,
		PRESENT
	};
	std::vector<QueueFamily> queue_families;
	std::optional<SwapChainSupportDetails> supported_swap_chain_features;

	bool supports_required_extensions()
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

	int score(const VkSurfaceKHR& surface)
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(physical_device, &device_properties);

		VkPhysicalDeviceFeatures dev_features;
		vkGetPhysicalDeviceFeatures(physical_device, &dev_features);

		if (queue_families.empty()) {
			queue_families = find_queue_families(surface, physical_device);
		}
		if (!supported_swap_chain_features.has_value() && supports_required_extensions()) {
			supported_swap_chain_features = querySwapChainSupport(physical_device, surface);
		}

		int score = 1000 * (int)(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
		score += device_properties.limits.maxImageDimension2D;

		bool has_required_features = queue_families[Device::GRAPHICS].index.has_value()
			&& queue_families[Device::PRESENT].index.has_value()
			&& !supported_swap_chain_features.value().formats.empty()
			&& !supported_swap_chain_features.value().present_modes.empty()
			&& dev_features.geometryShader;

		return score * (int)has_required_features;
	}
};

std::vector<QueueFamily> find_queue_families(const VkSurfaceKHR& surface, const VkPhysicalDevice& device)
{
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
	std::vector<QueueFamily> families(queue_family_count, QueueFamily {});

	for (uint32_t i = 0; i < queue_family_count; ++i) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			families[Device::GRAPHICS].index = i;

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
		if (present_support)
			families[Device::PRESENT].index = i;
	}

	return families;
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

void pick_physical_device(
	const VkInstance& instance,
	VkSurfaceKHR& surface,
	Device& device)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0)
		throw std::runtime_error("failed to find GPU with Vulkan Support");
	std::vector<VkPhysicalDevice> physical_devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());
	std::vector<Device> devices(device_count);

	std::map<int, Device> candidates;
	for (size_t i = 0; i < device_count; ++i) {
		devices[i].physical_device = physical_devices[i];
		candidates[devices[i].score(surface)] = devices[i];
	}
	auto best_device = *candidates.rbegin();
	if (best_device.first > 0)
		device = best_device.second;

	if (device.physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find suitable GPU");
	}
}

void init_logical_device(Device& device)
{
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {
		device.queue_families[Device::GRAPHICS].index.value(),
		device.queue_families[Device::PRESENT].index.value(),
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

	if (vkCreateDevice(device.physical_device, &create_info, nullptr, &device.logical_device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");

	// creating logical device also create queues for it automatically
	// only used one queue, so only need 0 for index
	vkGetDeviceQueue(
		device.logical_device,
		device.queue_families[Device::GRAPHICS].index.value(),
		0,
		&device.queue_families[Device::GRAPHICS].queue);
	vkGetDeviceQueue(
		device.logical_device,
		device.queue_families[Device::PRESENT].index.value(),
		0,
		&device.queue_families[Device::PRESENT].queue);
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
	const Device& device,
	const VkSurfaceKHR& surface,
	SwapChain& swap_chain)
{
	swap_chain.support_details = device.supported_swap_chain_features.value();
	auto surface_format = choose_swap_chain_surface_format(swap_chain.support_details.formats);
	auto present_mode = choose_swap_chain_present_mode(swap_chain.support_details.present_modes);
	auto extent = choose_swap_chain_extent(window, swap_chain.support_details.capabilities);

	uint32_t image_count = swap_chain.support_details.capabilities.minImageCount + 1;
	if (swap_chain.support_details.capabilities.maxImageCount > 0)
		image_count = std::min(image_count, swap_chain.support_details.capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto g_index = device.queue_families[Device::GRAPHICS].index.value();
	auto p_index = device.queue_families[Device::PRESENT].index.value();
	uint32_t queue_family_indices[] = {
		g_index,
		p_index
	};

	if (g_index != p_index) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swap_chain.support_details.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(
			device.logical_device,
			&create_info,
			nullptr,
			&swap_chain.swap_chain)
		!= VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");

	vkGetSwapchainImagesKHR(
		device.logical_device,
		swap_chain.swap_chain,
		&image_count,
		nullptr);
	swap_chain.images.resize(image_count);
	vkGetSwapchainImagesKHR(
		device.logical_device,
		swap_chain.swap_chain,
		&image_count,
		swap_chain.images.data());
	swap_chain.format = surface_format.format;
	swap_chain.extent = extent;
}

void init_image_views(const Device& device, SwapChain& swap_chain)
{
	swap_chain.image_views.resize(swap_chain.images.size());
	for (size_t i = 0; i < swap_chain.images.size(); ++i) {
		VkImageViewCreateInfo create_info {};
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

		if (vkCreateImageView(
				device.logical_device,
				&create_info,
				nullptr,
				&swap_chain.image_views[i])
			!= VK_SUCCESS)
			throw std::runtime_error("failed to create image view");
	}
}

VkShaderModule create_shader_module(const Device& device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module {};
	if (vkCreateShaderModule(device.logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module");

	return shader_module;
}

void init_render_pass(const Device& device, const SwapChain& swap_chain, VkRenderPass& render_pass) {
	VkAttachmentDescription color_attachment{};
	color_attachment.format = swap_chain.format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkRenderPassCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.attachmentCount = 1;
	create_info.pAttachments = &color_attachment;
	create_info.subpassCount = 1;
	create_info.pSubpasses = &subpass;

	if (vkCreateRenderPass(device.logical_device, &create_info, nullptr, &render_pass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass");
}

void init_graphics_pipeline(
		const std::filesystem::path& root,
		const Device& device,
		const SwapChain& swap_chain,
		const VkRenderPass& render_pass,
		VkPipelineLayout& pipeline_layout,
		VkPipeline& graphics_pipeline)
{
	auto vert_shader_file = read_file(root, "shaders/shader_vert.spv");
	auto frag_shader_file = read_file(root, "shaders/shader_frag.spv");

	auto vert_shader_mod = create_shader_module(device, vert_shader_file);
	auto frag_shader_mod = create_shader_module(device, frag_shader_file);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_mod;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_mod;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		vert_shader_stage_info,
		frag_shader_stage_info
	};

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	VkPipelineVertexInputStateCreateInfo vertex_input_info{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 0;
	vertex_input_info.pVertexBindingDescriptions = nullptr;
	vertex_input_info.vertexAttributeDescriptionCount = 0;
	vertex_input_info.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swap_chain.extent.width;
	viewport.height = (float) swap_chain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swap_chain.extent;

	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pSetLayouts = nullptr;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(
				device.logical_device,
				&pipeline_layout_info,
				nullptr,
				&pipeline_layout)
			!= VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout");

	VkGraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = nullptr;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = &dynamic_state;
	pipeline_info.layout = pipeline_layout;
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(
				device.logical_device,
				VK_NULL_HANDLE,
				1,
				&pipeline_info,
				nullptr,
				&graphics_pipeline)
			!= VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline");

	vkDestroyShaderModule(device.logical_device, vert_shader_mod, nullptr);
	vkDestroyShaderModule(device.logical_device, frag_shader_mod, nullptr);
}

int main(int argc, char** argv)
{
	(void)argc;
	auto root = std::filesystem::absolute(argv[0]).parent_path();

	GLFWwindow* window = nullptr;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
	VkSurfaceKHR surface;

	Device device;
	SwapChain swap_chain;
	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	try {
		init_window(window);
		init_instance(instance);
		init_debugger(instance, debug_messenger);
		init_surface(instance, window, surface);

		// these should be in a device.init() function
		pick_physical_device(instance, surface, device);
		init_logical_device(device);

		init_swap_chain(window, device, surface, swap_chain);
		init_image_views(device, swap_chain);
		init_render_pass(device, swap_chain, render_pass);
		init_graphics_pipeline(
				root,
				device,
				swap_chain,
				render_pass,
				pipeline_layout,
				graphics_pipeline);

		// main loop
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}

		// cleanup, reverse order of initiliztion
		vkDestroyPipeline(device.logical_device, graphics_pipeline, nullptr);
		vkDestroyPipelineLayout(device.logical_device, pipeline_layout, nullptr);
		vkDestroyRenderPass(device.logical_device, render_pass, nullptr);
		for (auto view : swap_chain.image_views)
			vkDestroyImageView(device.logical_device, view, nullptr);
		vkDestroySwapchainKHR(device.logical_device, swap_chain.swap_chain, nullptr);
		vkDestroyDevice(device.logical_device, nullptr);
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
