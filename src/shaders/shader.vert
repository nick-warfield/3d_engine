#version 450

// Push constants would be a better choice for MVP transformation instead of UBOs
// eg: layout(push_constant) uniform MVPTransform...

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;

void main() {
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 1.0);
	frag_color = in_color;
	frag_tex_coord = tex_coord;
}
