#version 450

// Push constants would be a better choice for MVP transformation instead of UBOs
// eg: layout(push_constant) uniform MVPTransform...

layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 tex_coord;

layout(location = 0) out vec2 frag_tex_coord;

void main() {
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 1.0);
	frag_tex_coord = tex_coord;
}
