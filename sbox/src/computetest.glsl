#version 440

#compute

layout (local_size_x = 256) in;

layout (binding = 0) uniform Config {
	uint number_count;
} config;

layout (binding = 1) readonly buffer InputBuffer {
	float numbers[];
} in_buf;

layout (binding = 2) buffer OutBuffer {
	float numbers[];
} out_buf;

void main() {
	uint id = gl_GlobalInvocationID.x;

	if (id < config.number_count) {
		out_buf.numbers[id] = in_buf.numbers[id] + 5.0f;
	}
}
