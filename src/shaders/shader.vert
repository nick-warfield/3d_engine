#version 450

// Push constants would be a better choice for MVP transformation instead of UBOs
// eg: layout(push_constant) uniform MVPTransform...

layout(set = 0, binding = 0) uniform MVP {
	mat4 model;
	mat4 view;
	mat4 projection;
} camera;

layout(set = 1, binding = 0) uniform Transform {
	mat4 position;
	mat4 rotation;
	mat4 scale;
} transform;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 tex_coord;

layout(location = 0) out vec2 frag_tex_coord;

void main() {
	mat4 mvp = camera.projection * camera.view * camera.model;
	mat4 trans = transform.scale * transform.rotation * transform.position;

	gl_Position = mvp * trans * vec4(in_position, 1.0);
	frag_tex_coord = tex_coord;
}
