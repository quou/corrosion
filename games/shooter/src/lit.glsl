#version 440

#begin VERTEX

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (std140, set = 0, binding = 0) uniform VertexConfig {
	mat4 view, projection;
};

layout (std140, set = 0, binding = 2) uniform Transforms {
	mat4 transforms[1000];
};

layout (location = 0) out VSOut {
	vec3 normal;
	vec3 world_pos;
} vs_out;

void main() {
	mat4 transform = transforms[gl_InstanceIndex];

	vs_out.normal = mat3(transpose(inverse(transform))) * normal;
	vs_out.world_pos = vec3(transform * vec4(position, 1.0));

	gl_Position = projection * view * vec4(vs_out.world_pos, 1.0);
}

#end VERTEX

#begin FRAGMENT

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec3 normal;
	vec3 world_pos;
} fs_in;

layout (std140, set = 0, binding = 1) uniform FragmentConfig {
	vec3 camera_pos;
} config;

void main() {
	colour.rgb = vec3(1.0) * dot(normalize(config.camera_pos - fs_in.world_pos), normalize(fs_in.normal));
	colour.a = 1.0;
}

#end FRAGMENT
