#version 440

#compute

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout (binding = 0) uniform Config {
	vec3 image_size;
	uint point_count;
} config;

layout (binding = 1) buffer InputPoints {
	vec3 points[];
};

layout (binding = 2, r16f) uniform image3D output_texture;

void main() {
	ivec3 id = ivec3(gl_GlobalInvocationID.xyz);
	vec3 pos = vec3(id);
	vec3 coord = pos / config.image_size;

	float dist = 1.0 / 0.0;

	for (uint i = 0; i < config.point_count; i++) {
		float d = length(coord - points[i]);

		if (d < dist) {
			dist = d;
		}
	}

	imageStore(output_texture, id, vec4(dist));
}
