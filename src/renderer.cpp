#include "renderer.hpp"
#include "buffer.hpp"
#include "constants.hpp"
#include "device.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "util.hpp"
#include "vertex.hpp"
#include "window.hpp"

#include <array>
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

void Renderer::init(const Window& window, const Device& device)
{
	glfwSetWindowUserPointer(window.glfw_window, &framebuffer_resized);
	glfwSetFramebufferSizeCallback(window.glfw_window, framebuffer_resize_callback);

	init_swap_chain(device, window);

	auto dev = device.logical_device;
	init_image_views(dev);
	init_render_pass(device);
	init_depth_image(device);
	init_msaa_image(device);
	init_framebuffers(dev);
	init_base_descriptor(device);

	frames.init(device);
}

void Renderer::deinit(const VkDevice& device, const VkAllocationCallbacks* pAllocator)
{
	frames.deinit(device, pAllocator);
	depth_image.deinit(device, pAllocator);
	msaa_image.deinit(device, pAllocator);

	for (auto framebuffer : framebuffers)
		vkDestroyFramebuffer(device, framebuffer, pAllocator);

	vkDestroyRenderPass(device, render_pass, pAllocator);

	uniform.deinit(device, pAllocator);
	texture.deinit(device, pAllocator);
	vkDestroyDescriptorPool(device, descriptor_pool, pAllocator);
	vkDestroyDescriptorSetLayout(device, descriptor_set_layout, pAllocator);

	for (auto view : swap_chain_image_views)
		vkDestroyImageView(device, view, pAllocator);
	vkDestroySwapchainKHR(device, swap_chain, pAllocator);
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
	if (queue_family_indices.size() > 1) {
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

void Renderer::init_depth_image(const Device& device)
{
	VkFormat depth_format = find_supported_format(
		device,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	depth_image.init(
		device,
		extent.width,
		extent.height,
		1,
		device.msaa_samples,
		depth_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Renderer::init_msaa_image(const Device& device)
{
	msaa_image.init(
		device,
		extent.width,
		extent.height,
		1,
		device.msaa_samples,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::init_render_pass(const Device& device)
{
	VkAttachmentDescription color_attachment {};
	color_attachment.format = format;
	color_attachment.samples = device.msaa_samples;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription color_attachment_resolve {};
	color_attachment_resolve.format = format;
	color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_resolve_ref {};
	color_attachment_resolve_ref.attachment = 2;
	color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depth_attachment {};
	depth_attachment.format = find_supported_format(
		device,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depth_attachment.samples = device.msaa_samples;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	subpass.pResolveAttachments = &color_attachment_resolve_ref;

	VkSubpassDependency dependency {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = {
		color_attachment,
		depth_attachment,
		color_attachment_resolve
	};
	VkRenderPassCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	create_info.pAttachments = attachments.data();
	create_info.subpassCount = 1;
	create_info.pSubpasses = &subpass;
	create_info.dependencyCount = 1;
	create_info.pDependencies = &dependency;

	if (vkCreateRenderPass(device.logical_device, &create_info, nullptr, &render_pass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass");
}

void Renderer::init_framebuffers(const VkDevice& device)
{
	framebuffers.resize(swap_chain_image_views.size());

	for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
		std::array<VkImageView, 3> attachments = {
			msaa_image.image_view,
			depth_image.image_view,
			swap_chain_image_views[i]
		};

		VkFramebufferCreateInfo framebuffer_info {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = extent.width;
		framebuffer_info.height = extent.height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}

void Renderer::init_base_descriptor(const Device& device)
{
	descriptor_pool = make_descriptor_pool(device.logical_device);
	descriptor_set_layout = make_default_descriptor_layout(device.logical_device);

	texture.init(device, "viking_room.png");
	uniform.init(device);

	descriptor_set = make_descriptor_set(
		device.logical_device,
		descriptor_pool,
		descriptor_set_layout,
		texture,
		uniform);
}

void Renderer::record_command_buffer(
	VkCommandBuffer& command_buffer,
	const Mesh& mesh,
	const Material& material)
{
	vkCmdBindDescriptorSets(
		command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		material.pipeline_layout,
		0,
		1,
		&descriptor_set[frames.index],
		0,
		nullptr);

	vkCmdBindDescriptorSets(
		command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		material.pipeline_layout,
		1,
		1,
		&material.descriptor_set[frames.index],
		0,
		nullptr);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);

	VkBuffer vertex_buffers[] = { mesh.vertex_buffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, mesh.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

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

	vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
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
	depth_image.deinit(device.logical_device, nullptr);
	msaa_image.deinit(device.logical_device, nullptr);
	for (auto& framebuffer : framebuffers)
		vkDestroyFramebuffer(device.logical_device, framebuffer, nullptr);
	for (auto& iv : swap_chain_image_views)
		vkDestroyImageView(device.logical_device, iv, nullptr);
	vkDestroySwapchainKHR(device.logical_device, swap_chain, nullptr);

	device.update_swap_chain_support_info(window);
	init_swap_chain(device, window);
	init_image_views(device.logical_device);
	init_depth_image(device);
	init_msaa_image(device);
	init_framebuffers(device.logical_device);
}

void Renderer::setup_draw(Window& window, Device& device)
{
	auto frame = frames.current_frame();
	vkWaitForFences(device.logical_device, 1, &frame.in_flight_fence, VK_TRUE, UINT64_MAX);

	auto result = vkAcquireNextImageKHR(
		device.logical_device,
		swap_chain,
		UINT64_MAX,
		frame.image_available_semaphore,
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

	vkResetFences(device.logical_device, 1, &frame.in_flight_fence);
	vkResetCommandBuffer(frame.command_buffer, 0);

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(frame.command_buffer, &begin_info) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer");

	VkRenderPassBeginInfo render_pass_info {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.framebuffer = framebuffers[image_index];
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = extent;

	std::array<VkClearValue, 2> clear_values {};
	clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clear_values[1].depthStencil = { 1.0f, 0 };

	render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
	render_pass_info.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(frame.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::draw(Mesh& mesh, Material& material)
{
	record_command_buffer(
		frames.current_frame().command_buffer,
		mesh,
		material);
}

void Renderer::present_draw(Window& window, Device& device)
{
	auto frame = frames.current_frame();

	vkCmdEndRenderPass(frame.command_buffer);

	if (vkEndCommandBuffer(frame.command_buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer");

	VkSubmitInfo submit_info {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { frame.image_available_semaphore };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &frame.command_buffer;

	VkSemaphore signal_semaphores[] = { frame.render_finished_semaphore };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	if (vkQueueSubmit(
			device.graphics_queue_family.queue,
			1,
			&submit_info,
			frame.in_flight_fence)
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

	auto result = vkQueuePresentKHR(device.present_queue_family.queue, &present_info);

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

	frames.next();
}

}
