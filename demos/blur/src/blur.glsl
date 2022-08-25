#version 440

#compute

layout (local_size_x = 16, local_size_y = 16) in;

layout (binding = 0, rgba16f) uniform readonly image2D input_image;
layout (binding = 1, rgba16f) uniform image2D output_image;

void main() {
	int blur_amt = 3;

	vec4 accum = vec4(0.0);
	float sample_count = 0.0;
	for (int y = -4; y < 8; y++) {
		for (int x = -4; x < 8; x++) {
			accum += imageLoad(input_image, ivec2(gl_GlobalInvocationID.x + x * blur_amt, gl_GlobalInvocationID.y + y * blur_amt));
			sample_count++;
		}
	}

	imageStore(output_image, ivec2(gl_GlobalInvocationID.xy), accum / sample_count);
}
