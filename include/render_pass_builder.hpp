#pragma once

#include "context.hpp"
#include <map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <limits>

namespace chch {

struct RenderPassBuilder {
	struct SubpassBuilder {
		friend RenderPassBuilder;

		SubpassBuilder add_color_ref(
				uint32_t color_index,
				uint32_t resolve_index = std::numeric_limits<uint32_t>::max());
		SubpassBuilder add_depth_ref(uint32_t attachment_index);
		SubpassBuilder add_input_ref(uint32_t attachment_index);
		SubpassBuilder preserve_attachment(uint32_t attachment_index);
		RenderPassBuilder end_subpass();

	private:
		RenderPassBuilder* m_render_pass_builder;
		uint32_t m_subpass_index;
		std::vector<VkAttachmentReference> m_color_ref;
		std::vector<VkAttachmentReference> m_color_resolve_ref;
		std::vector<VkAttachmentReference> m_depth_ref;
		std::vector<VkAttachmentReference> m_input_ref;
		std::vector<uint32_t> m_preserve_ref;
	};

	static RenderPassBuilder begin(const Context* context) {
		RenderPassBuilder builder {};
		builder.m_context = context;
		return builder;
	}

	VkResult build(VkRenderPass* render_pass);

	RenderPassBuilder add_color_attachment(uint32_t attachment_index, VkFormat format);
	RenderPassBuilder add_color_resolve_attachment(uint32_t attachment_index, VkFormat format);
	RenderPassBuilder add_depth_attachment(
		uint32_t attachment_index,
		VkSampleCountFlagBits samples,
		VkAttachmentStoreOp store_op,
		VkImageLayout final_layout);
	RenderPassBuilder add_input_attachment(uint32_t attachment_index);
	SubpassBuilder begin_subpass(uint32_t subpass_index);
	RenderPassBuilder add_dependency(
		uint32_t src_subpass_index,
		VkPipelineStageFlags src_stage_mask,
		VkAccessFlags src_access_mask,
		uint32_t dst_subpass_index,
		VkPipelineStageFlags dst_stage_mask,
		VkAccessFlags dst_access_mask);

private:
	const Context* m_context;
	std::map<uint32_t, VkAttachmentDescription> m_attachments;
	std::map<uint32_t, VkSubpassDescription> m_subpasses;
	std::vector<VkSubpassDependency> m_depends;
};

}
