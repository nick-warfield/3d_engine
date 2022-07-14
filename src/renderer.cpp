#include "renderer.hpp"
#include "constants.hpp"
#include "device.hpp"
#include "uniform_buffer_object.hpp"
#include "vertex.hpp"
#include "window.hpp"
#include "buffer.hpp"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <set>

namespace gfx {

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
	auto framebuffer_resized = reinterpret_cast<bool*>(glfwGetWindowUserPointer(window));
	*framebuffer_resized = true;
	(void)width;
	(void)height;
}

void Renderer::init(
		const Window& window,
		const Device& device,
		const std::vector<Buffer>& uniform_buffers)
{
	glfwSetWindowUserPointer(window.glfw_window, &framebuffer_resized);
	glfwSetFramebufferSizeCallback(window.glfw_window, framebuffer_resize_callback);

	init_swap_chain(device, window);

	auto dev = device.logical_device;
	init_image_views(dev);
	init_render_pass(dev);
	init_framebuffers(dev);
	init_descriptor_sets(dev, uniform_buffers);
	init_graphics_pipeline(dev);
	init_command_buffers(device);
	init_sync_objects(dev);
}

void Renderer::deinit(const Device& device, const VkAllocationCallbacks* pAllocator)
{
	auto dev = device.logical_device;

	for (auto& semaphore : image_available_semaphores)
		vkDestroySemaphore(dev, semaphore, pAllocator);
	for (auto& semaphore : render_finished_semaphores)
		vkDestroySemaphore(dev, semaphore, pAllocator);
	for (auto& fence : in_flight_fences)
		vkDestroyFence(dev, fence, pAllocator);

	vkDestroyCommandPool(dev, graphics_command_pool, pAllocator);

	for (auto framebuffer : framebuffers)
		vkDestroyFramebuffer(dev, framebuffer, pAllocator);

	vkDestroyPipeline(dev, graphics_pipeline, pAllocator);
	vkDestroyRenderPass(dev, render_pass, pAllocator);
	vkDestroyPipelineLayout(dev, pipeline_layout, pAllocator);

	vkDestroyDescriptorPool(dev, descriptor_pool, pAllocator);
	vkDestroyDescriptorSetLayout(dev, descriptor_set_layout, pAllocator);

	for (auto view : swap_chain_image_views)
		vkDestroyImageView(dev, view, pAllocator);
	vkDestroySwapchainKHR(dev, swap_chain, pAllocator);
}

void Renderer::init_swap_chain(const Device& device, const Window& window)
{
	auto choose_swap_chain_surface_format = [](const std::vector<VkSurfaceFormatKHR>& available_formats) {
		for (const auto& a_format : available_formats) {
			if (a_format.format == VK_FORMAT_B8G8R8A8_SRGB
				&& a_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return a_format;
			}
		}
		return available_formats[0];
	};

	auto choose_swap_chain_present_mode = [](const std::vector<VkPresentModeKHR>& available_present_modes) {
		for (const auto& a_mode : available_present_modes) {
			if (a_mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return a_mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	};

	auto choose_swap_chain_extent = [](GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
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
	};

	auto support_info = device.supported_swap_chain_features;
	auto surface_format = choose_swap_chain_surface_format(support_info.formats);
	auto present_mode = choose_swap_chain_present_mode(support_info.present_modes);
	extent = choose_swap_chain_extent(window.glfw_window, support_info.capabilities);
	format = surface_format.format;

	uint32_t image_count = support_info.capabilities.minImageCount + 1;
	if (support_info.capabilities.maxImageCount > 0)
		image_count = std::min(image_count, support_info.capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = window.surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto queue_family_indices = device.get_unique_queue_family_indices();
	if (queue_family_indices.size() > 0) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = queue_family_indices.size();
		create_info.pQueueFamilyIndices = queue_family_indices.data();
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = support_info.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	auto dev = device.logical_device;
	if (vkCreateSwapchainKHR(dev, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");

	vkGetSwapchainImagesKHR(dev, swap_chain, &image_count, nullptr);
	swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(dev, swap_chain, &image_count, swap_chain_images.data());
}

void Renderer::init_image_views(const VkDevice& device)
{
	swap_chain_image_views.resize(swap_chain_images.size());
	for (size_t i = 0; i < swap_chain_images.size(); ++i) {
		VkImageViewCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = swap_chain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = format;

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
				device,
				&create_info,
				nullptr,
				&swap_chain_image_views[i])
			!= VK_SUCCESS)
			throw std::runtime_error("failed to create image view");
	}
}

void Renderer::init_render_pass(const VkDevice& device)
{
	VkAttachmentDescription color_attachment {};
	color_attachment.format = format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkSubpassDependency dependency {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.attachmentCount = 1;
	create_info.pAttachments = &color_attachment;
	create_info.subpassCount = 1;
	create_info.pSubpasses = &subpass;
	create_info.dependencyCount = 1;
	create_info.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &create_info, nullptr, &render_pass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass");
}

void Renderer::init_descriptor_sets(
		const VkDevice& device,
		const std::vector<Buffer>& uniform_buffers)
{
	// Create Descriptor Set Layout
	VkDescriptorSetLayoutBinding ubo_layout_binding {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layout_info {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = 1;
	layout_info.pBindings = &ubo_layout_binding;

	if (vkCreateDescriptorSetLayout(
				device,
				&layout_info,
				nullptr,
				&descriptor_set_layout)
			!= VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout");

	// Allocate Descriptor Pool
	VkDescriptorPoolSize pool_size {};
	pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_size.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool");

	// Create Descriptor Sets
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);
	VkDescriptorSetAllocateInfo alloc_info;
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool;
	alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	alloc_info.pSetLayouts = layouts.data();
	alloc_info.pNext = nullptr;

	descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets");

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkDescriptorBufferInfo buffer_info {};
		buffer_info.buffer = uniform_buffers[i].buffer;
		buffer_info.offset = 0;
		buffer_info.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptor_write {};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = descriptor_sets[i];
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pBufferInfo = &buffer_info;
		descriptor_write.pImageInfo = nullptr;
		descriptor_write.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
	}
}

void Renderer::init_graphics_pipeline(const VkDevice& device)
{
	auto read_file = [](const char* filename) {
		std::ifstream file((Root::path / filename).string(), std::ios::ate | std::ios::binary);
		if (!file.is_open())
			throw std::runtime_error("failed to open file");

		size_t file_size = (size_t)file.tellg();
		std::vector<char> buffer(file_size);

		file.seekg(0);
		file.read(buffer.data(), file_size);
		file.close();

		return buffer;
	};

	auto vert_shader_file = read_file("shaders/shader_vert.spv");
	auto frag_shader_file = read_file("shaders/shader_frag.spv");

	auto create_shader_module = [](const VkDevice& device, const std::vector<char>& code) {
		VkShaderModuleCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shader_module {};
		if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
			throw std::runtime_error("failed to create shader module");

		return shader_module;
	};

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

	VkPipelineDynamicStateCreateInfo dynamic_state {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	auto binding_description = Vertex::get_binding_description();
	auto attribute_description = Vertex::get_attribute_description();

	VkPipelineVertexInputStateCreateInfo vertex_input_info {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_description.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_description.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	//	multisampling.minSampleShading = 1.0f;
	//	multisampling.pSampleMask = nullptr;
	//	multisampling.alphaToCoverageEnable = VK_FALSE;
	//	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	//	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	//	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	//	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	//	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipeline_layout_info {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
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

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline)
		!= VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline");

	vkDestroyShaderModule(device, vert_shader_mod, nullptr);
	vkDestroyShaderModule(device, frag_shader_mod, nullptr);
}

void Renderer::init_framebuffers(const VkDevice& device)
{
	framebuffers.resize(swap_chain_image_views.size());

	for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
		VkImageView attachments[] = { swap_chain_image_views[i] };

		VkFramebufferCreateInfo framebuffer_info {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = extent.width;
		framebuffer_info.height = extent.height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}

void Renderer::init_command_buffers(const Device& device)
{
	auto dev = device.logical_device;
	VkCommandPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = device.graphics_queue_family.index.value();

	if (vkCreateCommandPool(dev, &pool_info, nullptr, &graphics_command_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = graphics_command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateCommandBuffers(dev, &alloc_info, command_buffers.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffer");
}

void Renderer::init_sync_objects(const VkDevice& device)
{
	image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphore_info {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	bool success = true;
	for (auto& s : image_available_semaphores)
		success = success && vkCreateSemaphore(device, &semaphore_info, nullptr, &s) == VK_SUCCESS;
	for (auto& s : render_finished_semaphores)
		success = success && vkCreateSemaphore(device, &semaphore_info, nullptr, &s) == VK_SUCCESS;
	for (auto& fence : in_flight_fences)
		success = success && vkCreateFence(device, &fence_info, nullptr, &fence) == VK_SUCCESS;

	if (!success)
		throw std::runtime_error("failed to create synchronization objects");
}

void Renderer::record_command_buffer(
		VkCommandBuffer& command_buffer,
		const VkDescriptorSet& descriptor_set,
		const VkBuffer& vertex_buffer,
		const VkBuffer& index_buffer,
		int image_index)
{
	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer");

	VkRenderPassBeginInfo render_pass_info {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.framebuffer = framebuffers[image_index];
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = extent;

	VkClearValue clear_color = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

	VkBuffer vertex_buffers[] = { vertex_buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);

	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor {};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(
			command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline_layout,
			0,
			1,
			&descriptor_set,
			0,
			nullptr);

	vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	vkCmdEndRenderPass(command_buffer);

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer");
}

void Renderer::recreate_swap_chain(Window& window, Device& device)
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window.glfw_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window.glfw_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device.logical_device);

	// Need to cleanup old stuff first
	for (auto& framebuffer : framebuffers)
		vkDestroyFramebuffer(device.logical_device, framebuffer, nullptr);
	for (auto& iv : swap_chain_image_views)
		vkDestroyImageView(device.logical_device, iv, nullptr);
	vkDestroySwapchainKHR(device.logical_device, swap_chain, nullptr);

	device.update_swap_chain_support_info(window);
	init_swap_chain(device, window);
	init_image_views(device.logical_device);
	init_framebuffers(device.logical_device);
}

void Renderer::draw(
	Window& window,
	Device& device,
	BufferData& buffers)
{
	vkWaitForFences(device.logical_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

	uint32_t image_index;
	auto result = vkAcquireNextImageKHR(
		device.logical_device,
		swap_chain,
		UINT64_MAX,
		image_available_semaphores[current_frame],
		VK_NULL_HANDLE,
		&image_index);

	switch (result) {
	case VK_ERROR_OUT_OF_DATE_KHR:
		recreate_swap_chain(window, device);
		return;
	case VK_SUBOPTIMAL_KHR:
		break;
	case VK_SUCCESS:
		break;
	default:
		throw std::runtime_error("failed to acquire swap chain images");
		break;
	}

	vkResetFences(device.logical_device, 1, &in_flight_fences[current_frame]);

	buffers.update_uniform_buffer(device.logical_device, extent, current_frame);
	vkResetCommandBuffer(command_buffers[current_frame], 0);
	record_command_buffer(
			command_buffers[current_frame],
			descriptor_sets[current_frame],
			buffers.vertex_buffer.buffer,
			buffers.index_buffer.buffer,
			image_index);

	VkSubmitInfo submit_info {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { image_available_semaphores[current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers[current_frame];

	VkSemaphore signal_semaphores[] = { render_finished_semaphores[current_frame] };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	if (vkQueueSubmit(
			device.graphics_queue_family.queue,
			1,
			&submit_info,
			in_flight_fences[current_frame])
		!= VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer");

	VkPresentInfoKHR present_info;
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swap_chains[] = { swap_chain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

	// need to figure out why setting pNext got vkQueuePresentKHR to work
	present_info.pNext = nullptr;

	result = vkQueuePresentKHR(device.graphics_queue_family.queue, &present_info);

	switch (result) {
	case VK_SUCCESS:
		if (!framebuffer_resized)
			break;
		// else roll over
	case VK_SUBOPTIMAL_KHR:
		// roll over
	case VK_ERROR_OUT_OF_DATE_KHR:
		framebuffer_resized = false;
		recreate_swap_chain(window, device);
		break;
	default:
		throw std::runtime_error("failed to present swap chain images");
		break;
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

}
