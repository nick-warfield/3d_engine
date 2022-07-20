#version 450

// Push constants would be a better choice for MVP transformation instead of UBOs
// eg: layout(push_constant) uniform MVPTransform...

layout(set = 0, binding = 0) uniform SceneData {
	vec3 sun_color;
	vec3 sun_dir;
	float intensity;
	vec3 ambient_color;
} scene;

layout(push_constant) uniform constants { mat4 matrix; } mvp;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 f_normal;
layout(location = 1) out vec2 f_uv;

void main() {
	gl_Position = mvp.matrix * vec4(position, 1.0);

	f_normal = normal;
	f_uv = uv;
}
