#version 440

#begin VERTEX

/* Mesh data */
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

/* Instance data. Transformation matrix passed as
 * individual rows, as vertex attributes cannot be
 * sized higher than vec4. */
layout (location = 2) in vec4 r0;
layout (location = 3) in vec4 r1;
layout (location = 4) in vec4 r2;
layout (location = 5) in vec4 r3;

layout (std140, set = 0, binding = 0) uniform VertexConfig {
	mat4 view, projection;
};

layout (location = 0) out VSOut {
	vec3 normal;
	vec3 world_pos;
} vs_out;

void main() {
	mat4 transform;
	transform[0] = r0;
	transform[1] = r1;
	transform[2] = r2;
	transform[3] = r3;

	vs_out.normal = normalize(vec3(transform * vec4(normal, 0.0)));
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
