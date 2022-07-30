#include "renderer.hpp"
#include "buffer.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "util.hpp"
#include "vertex.hpp"
#include "camera.hpp"
#include "material.hpp"

#include "render_pass_builder.hpp"
#include "descriptor_builder.hpp"

#include <iostream>
#include <vulkan/vulkan_core.h>
#include <cstdint>

namespace chch {


void Renderer::init(Context* p_context, SceneGlobals scene_globals, Camera* p_camera)
{
	context = p_context;
	init_swap_chain();

	init_image_views();
	init_render_pass();
	init_depth_image();
	init_msaa_image();
	init_framebuffers();

	scene_uniform.init(context, scene_globals);
	init_base_descriptor();

	camera = p_camera;
	frames.init(context);
}

void Renderer::deinit()
{
	frames.deinit(context);
	depth_image.deinit(context);
	msaa_image.deinit(context);

	for (auto framebuffer : framebuffers)
		vkDestroyFramebuffer(context->device, framebuffer, context->allocation_callbacks);

	vkDestroyRenderPass(context->device, render_pass, context->allocation_callbacks);

	scene_uniform.deinit(context);
	vkDestroyDescriptorPool(context->device, descriptor_pool, context->allocation_callbacks);
	for (auto& layout : descriptor_set_layout)
		vkDestroyDescriptorSetLayout(context->device, layout, context->allocation_callbacks);

	for (auto view : swap_chain_image_views)
		vkDestroyImageView(context->device, view, context->allocation_callbacks);
	vkDestroySwapchainKHR(context->device, swap_chain, context->allocation_callbacks);
}

void Renderer::init_swap_chain()
{
	uint32_t image_count = context->surface_capabilities.minImageCount + 1;
	if (context->surface_capabilities.maxImageCount > 0)
		image_count = std::min(image_count, context->surface_capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = context->surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = context->surface_format.format;
	create_info.imageColorSpace = context->surface_format.colorSpace;
	create_info.imageExtent = context->surface_capabilities.currentExtent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (context->unique_queue_indices.size() > 1) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = context->unique_queue_indices.size();
		create_info.pQueueFamilyIndices = context->unique_queue_indices.data();
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = context->surface_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = context->present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(context->device, &create_info, context->allocation_callbacks, &swap_chain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");

	vkGetSwapchainImagesKHR(context->device, swap_chain, &image_count, nullptr);
	swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(context->device, swap_chain, &image_count, swap_chain_images.data());
}

void Renderer::init_image_views()
{
	swap_chain_image_views.resize(swap_chain_images.size());
	for (size_t i = 0; i < swap_chain_images.size(); ++i) {
		VkImageViewCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = swap_chain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = context->surface_format.format;

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
				context->device,
				&create_info,
				nullptr,
				&swap_chain_image_views[i])
			!= VK_SUCCESS)
			throw std::runtime_error("failed to create image view");
	}
}

void Renderer::init_depth_image()
{
	VkFormat depth_format = find_supported_format(
		context,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	depth_image.init(
		context,
		context->surface_capabilities.currentExtent.width,
		context->surface_capabilities.currentExtent.height,
		1,
		context->msaa_samples,
		depth_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);
}

void Renderer::init_msaa_image()
{
	msaa_image.init(
		context,
		context->surface_capabilities.currentExtent.width,
		context->surface_capabilities.currentExtent.height,
		1,
		context->msaa_samples,
		context->surface_format.format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);
}

void Renderer::init_render_pass()
{
	auto r = RenderPassBuilder::begin(context)
		.add_color_attachment(0, context->surface_format.format)
		.add_color_resolve_attachment(1, context->surface_format.format)
		.add_depth_attachment(2)
		.begin_subpass(0)
			.add_color_ref(0, 1)
			.add_depth_ref(2)
		.end_subpass()
		.add_dependency(
				VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				0,
				0,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
		.build(&render_pass);
	vk_check(r, "Failed to create render pass");
}

void Renderer::init_framebuffers()
{
	framebuffers.resize(swap_chain_image_views.size());

	for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
		std::array<VkImageView, 3> attachments = {
			msaa_image.image_view,
			swap_chain_image_views[i],
			depth_image.image_view
		};

		VkFramebufferCreateInfo framebuffer_info {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = context->surface_capabilities.currentExtent.width;
		framebuffer_info.height = context->surface_capabilities.currentExtent.height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(context->device, &framebuffer_info, context->allocation_callbacks, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}

void Renderer::init_base_descriptor()
{
	descriptor_pool = make_descriptor_pool(context->device, 0, 1);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
	DescriptorBuilder::begin(context, descriptor_pool)
		.bind_uniform(0, &scene_uniform.buffer[i])
		.build(&descriptor_set_layout[i], &descriptor_set[i]);
	}
}

void Renderer::record_command_buffer(
	VkCommandBuffer& command_buffer,
	const Transform& transform,
	const Mesh& mesh,
	const Material& material)
{
	// really only need to bind this once
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

	glm::mat4 matrix = correction_matrix * camera->matrix() * transform.matrix();
	vkCmdPushConstants(
			command_buffer,
			material.pipeline_layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(glm::mat4),
			&matrix);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);

	VkBuffer vertex_buffers[] = { mesh.vertex_buffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, mesh.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(context->surface_capabilities.currentExtent.width);
	viewport.height = static_cast<float>(context->surface_capabilities.currentExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor {};
	scissor.offset = { 0, 0 };
	scissor.extent = context->surface_capabilities.currentExtent;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
}

void Renderer::recreate_swap_chain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(context->window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(context->window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(context->device);
	camera->width = width;
	camera->height = height;
	camera->cache_good = false;

	// Need to cleanup old stuff first
	depth_image.deinit(context);
	msaa_image.deinit(context);
	for (auto& framebuffer : framebuffers)
		vkDestroyFramebuffer(context->device, framebuffer, context->allocation_callbacks);
	for (auto& iv : swap_chain_image_views)
		vkDestroyImageView(context->device, iv, context->allocation_callbacks);
	vkDestroySwapchainKHR(context->device, swap_chain, context->allocation_callbacks);

	init_swap_chain();
	init_image_views();
	init_depth_image();
	init_msaa_image();
	init_framebuffers();
}

void Renderer::setup_draw()
{
	auto frame = frames.current_frame();
	vkWaitForFences(context->device, 1, &frame.in_flight_fence, VK_TRUE, UINT64_MAX);

	auto result = vkAcquireNextImageKHR(
		context->device,
		swap_chain,
		UINT64_MAX,
		frame.image_available_semaphore,
		VK_NULL_HANDLE,
		&image_index);

	switch (result) {
	case VK_ERROR_OUT_OF_DATE_KHR:
		recreate_swap_chain();
		return;
	case VK_SUBOPTIMAL_KHR:
		break;
	case VK_SUCCESS:
		break;
	default:
		throw std::runtime_error("failed to acquire swap chain images");
		break;
	}

	vkResetFences(context->device, 1, &frame.in_flight_fence);
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
	render_pass_info.renderArea.extent = context->surface_capabilities.currentExtent;

	std::array<VkClearValue, 3> clear_values {};
	clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clear_values[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clear_values[2].depthStencil = { 1.0f, 0 };

	render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
	render_pass_info.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(frame.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::draw(const Transform& transform, const Mesh& mesh, const Material& material)
{
	record_command_buffer(
		frames.current_frame().command_buffer,
		transform,
		mesh,
		material);
}

void Renderer::present_draw()
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
			context->graphics_queue.queue,
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

	auto result = vkQueuePresentKHR(context->present_queue.queue, &present_info);

	switch (result) {
	case VK_SUCCESS:
		if (!context->window_resized)
			break;
		// else roll over
	case VK_SUBOPTIMAL_KHR:
		// roll over
	case VK_ERROR_OUT_OF_DATE_KHR:
		context->window_resized = false;
		recreate_swap_chain();
		break;
	default:
		throw std::runtime_error("failed to present swap chain images");
		break;
	}

	frames.next();
}

}
