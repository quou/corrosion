#version 440

#begin vertex

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out VSOut {
	vec2 uv;
} vs_out;

void main() {
	vs_out.uv = uv;

	gl_Position = vec4(position, 1.0);
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec2 uv;
} fs_in;

layout (binding = 0) uniform sampler2D colours;
layout (binding = 1) uniform sampler2D normals;
layout (binding = 2) uniform sampler2D positions;

#define max_lights 500

struct PointLight {
	float intensity;
	float range;
	vec3 diffuse;
	vec3 specular;
	vec3 position;
};

layout (std140, set = 0, binding = 3) uniform LightingBuffer {
	uint light_count;
	vec3 camera_pos;
	PointLight lights[max_lights];
};

void main() {
	vec3 normal = texture(normals, fs_in.uv).rgb;
	vec3 position = texture(positions, fs_in.uv).rgb;
	colour = texture(colours, fs_in.uv);

	vec3 view_dir = normalize(camera_pos - position);

	vec3 lighting_result = vec3(0.0);
	for (int i = 0; i < light_count; i++) {
		PointLight light = lights[i];

		vec3 light_dir = normalize(light.position - position);
		vec3 reflect_dir = reflect(-light_dir, normal);

		float dist = length(light.position - position);

		float attenuation = 1.0 / (pow((dist / light.range) * 5.0, 2.0) + 1);

		vec3 diffuse =
			attenuation *
			light.diffuse *
			light.intensity *
			colour.rgb *
			max(dot(light_dir, normal), 0.0);

		vec3 specular =
			attenuation *
			light.specular *
			light.intensity *
			colour.rgb * /* TODO: Draw specular maps into another attachment. */
			pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);

		lighting_result += diffuse + specular;
	}

	colour = vec4(lighting_result, 1.0);
}

#end fragment
