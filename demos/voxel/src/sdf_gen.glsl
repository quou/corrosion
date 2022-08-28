#version 440

#compute

layout (local_size_x = 16, local_size_y = 16) in;

layout (binding = 0) uniform Config {
	ivec2 viewport_size;
} config;

layout (binding = 1, rgba8) uniform image2D result;

void main() {
	if (gl_GlobalInvocationID.x >= config.viewport_size.x || gl_GlobalInvocationID.y >= config.viewport_size.y) { return; }

	ivec2 coords = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);

	imageStore(result, coords, vec4(1.0, 0.0, 0.0, 1.0));
}
