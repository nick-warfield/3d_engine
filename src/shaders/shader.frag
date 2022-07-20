#version 450

layout(set = 0, binding = 0) uniform SceneData {
	vec3 sun_color;
	vec3 sun_dir;
	float intensity;
	vec3 ambient_color;
} scene;

layout(set = 1, binding = 0) uniform SpecularData {
	vec3 camera_pos;
	float width;
} spec_data;

layout(set = 1, binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 out_color;

void main() {
	vec3 ambient = scene.ambient_color;
	float intensity = 0.5;

	float cos_theta = clamp(dot(normal, scene.sun_dir), 0, 1);
	vec3 diffuse = scene.sun_color * intensity * cos_theta;
	
	vec3 c_pos = normalize(spec_data.camera_pos);
	vec3 reflection = reflect(-scene.sun_dir, normal);
	float cos_alpha = clamp(dot(c_pos, reflection), 0, 1);
	vec3 specular = scene.sun_color * intensity * pow(cos_alpha, 2);

	vec3 color = texture(tex_sampler, uv).rgb * (ambient + diffuse + specular);
	out_color = vec4(color, 1.0);
}
