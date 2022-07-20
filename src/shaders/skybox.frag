#version 450

layout(set = 1, binding = 1) uniform sampler2D tex_sampler;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec4 out_color;

void main() {
	// my skybox texture isn't perfect
	vec2 adjust = vec2(1.0 / 3.0, 0.5) - uv;
	out_color = texture(tex_sampler, uv + adjust / 500);
}
