#include "render_pass_builder.hpp"
#include "util.hpp"
#include <cstdint>

namespace chch {

RenderPassBuilder::SubpassBuilder RenderPassBuilder::SubpassBuilder::add_color_ref(
	uint32_t color_index, uint32_t resolve_index)
{
	VkAttachmentReference color_attachment_ref {};
	color_attachment_ref.attachment = color_index;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_color_ref.push_back(color_attachment_ref);
	if (resolve_index == std::numeric_limits<uint32_t>::max())
		return *this;

	VkAttachmentReference color_attachment_resolve_ref {};
	color_attachment_resolve_ref.attachment = resolve_index;
	color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_color_resolve_ref.push_back(color_attachment_resolve_ref);
	return *this;
}

RenderPassBuilder::SubpassBuilder RenderPassBuilder::SubpassBuilder::add_depth_ref(
	uint32_t attachment_index)
{
	VkAttachmentReference depth_ref {};
	depth_ref.attachment = attachment_index;
	depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_depth_ref.push_back(depth_ref);
	return *this;
}

// TODO: Implement when I need this
//RenderPassBuilder::SubpassBuilder RenderPassBuilder::SubpassBuilder::add_input_ref(
//	uint32_t attachment_index)
//{
//	VkAttachmentReference input_ref {};
//	input_ref.attachment = attachment_index;
//	input_ref.layout = ???;
//	m_input_ref.push_back(input_ref);
//	return *this;
//}

RenderPassBuilder::SubpassBuilder RenderPassBuilder::SubpassBuilder::preserve_attachment(
	uint32_t attachment_index)
{
	m_preserve_ref.push_back(attachment_index);
	return *this;
}

RenderPassBuilder RenderPassBuilder::SubpassBuilder::end_subpass()
{
	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(m_color_ref.size());
	subpass.pColorAttachments = m_color_ref.data();
	subpass.pResolveAttachments = m_color_resolve_ref.data();
	subpass.pDepthStencilAttachment = m_depth_ref.data();
	subpass.inputAttachmentCount = static_cast<uint32_t>(m_input_ref.size());
	subpass.pInputAttachments = m_input_ref.data();
	subpass.preserveAttachmentCount = static_cast<uint32_t>(m_preserve_ref.size());
	subpass.pPreserveAttachments = m_preserve_ref.data();

	m_render_pass_builder->m_subpasses[m_subpass_index] = subpass;
	return *m_render_pass_builder;
}

VkResult RenderPassBuilder::build(VkRenderPass* render_pass)
{
	std::vector<VkAttachmentDescription> attachments;
	for (auto [key, value] : m_attachments)
		attachments.push_back(value);

	std::vector<VkSubpassDescription> subpasses;
	for (auto [key, value] : m_subpasses)
		subpasses.push_back(value);

	VkRenderPassCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	create_info.pAttachments = attachments.data();
	create_info.subpassCount = static_cast<uint32_t>(subpasses.size());
	create_info.pSubpasses = subpasses.data();
	create_info.dependencyCount = static_cast<uint32_t>(m_depends.size());
	create_info.pDependencies = m_depends.data();

	return vkCreateRenderPass(
		m_context->device,
		&create_info,
		m_context->allocation_callbacks,
		render_pass);
}

RenderPassBuilder RenderPassBuilder::add_color_attachment(uint32_t attachment_index, VkFormat format)
{
	VkAttachmentDescription attachment {};
	attachment.format = format;
	attachment.samples = m_context->msaa_samples;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	m_attachments[attachment_index] = attachment;
	return *this;
}

RenderPassBuilder RenderPassBuilder::add_color_resolve_attachment(uint32_t attachment_index, VkFormat format)
{
	VkAttachmentDescription attachment {};
	attachment.format = format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	m_attachments[attachment_index] = attachment;
	return *this;
}

RenderPassBuilder RenderPassBuilder::add_depth_attachment(uint32_t attachment_index)
{
	VkAttachmentDescription attachment {};
	attachment.format = find_supported_format(
		m_context,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	attachment.samples = m_context->msaa_samples;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	m_attachments[attachment_index] = attachment;
	return *this;
}

// TODO: Implement when I need these
// RenderPassBuilder RenderPassBuilder::add_input_attachment(uint32_t attachment_index)
//{
//}

RenderPassBuilder::SubpassBuilder RenderPassBuilder::begin_subpass(uint32_t subpass_index)
{
	SubpassBuilder sub_builder;
	sub_builder.m_render_pass_builder = this;
	sub_builder.m_subpass_index = subpass_index;
	return sub_builder;
}

RenderPassBuilder RenderPassBuilder::add_dependency(
	uint32_t src_subpass_index,
	VkPipelineStageFlags src_stage_mask,
	VkAccessFlags src_access_mask,
	uint32_t dst_subpass_index,
	VkPipelineStageFlags dst_stage_mask,
	VkAccessFlags dst_access_mask)
{
	VkSubpassDependency dependency {};
	dependency.srcSubpass = src_subpass_index;
	dependency.srcStageMask = src_stage_mask;
	dependency.srcAccessMask = src_access_mask;

	dependency.dstSubpass = dst_subpass_index;
	dependency.dstStageMask = dst_stage_mask;
	dependency.dstAccessMask = dst_access_mask;

	m_depends.push_back(dependency);
	return *this;
}

}
