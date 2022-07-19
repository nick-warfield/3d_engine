#version 450

layout(set = 1, binding = 0) uniform Sun {
	vec3 color;
} c;

layout(location = 0) out vec4 out_color;

void main() {
	out_color = vec4(c.color, 1.0);
}
