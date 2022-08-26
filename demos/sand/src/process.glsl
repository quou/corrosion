#version 440

#compute

layout (local_size_x = 16, local_size_y = 16) in;

layout (binding = 0) uniform Config {
	ivec2 sim_size;
} config;

layout (binding = 1) buffer ParticleBuffer {
	int particles[];
};

layout (binding = 2) buffer TempParticleBuffer {
	int temp_particles[];
};

layout (binding = 3, rgba8) uniform image2D result;

void update_sand(ivec2 coords, uint idx) {
	groupMemoryBarrier();

	ivec2 bottom_coords = ivec2(coords.x, coords.y + 1);
	ivec2 bottom_left_coords = ivec2(coords.x - 1, coords.y + 1);
	ivec2 bottom_right_coords = ivec2(coords.x + 1, coords.y + 1);
	if (bottom_coords.y >= config.sim_size.y) {
		return;
	}

	if (bottom_left_coords.x < 0) {
		return;
	}

	if (bottom_right_coords.x >= config.sim_size.x) {
		return;
	}

	int bottom_idx = bottom_coords.x + bottom_coords.y * config.sim_size.x;
	int bottom_left_idx = bottom_left_coords.x + bottom_left_coords.y * config.sim_size.x;
	int bottom_right_idx = bottom_right_coords.x + bottom_right_coords.y * config.sim_size.x;

	if (temp_particles[bottom_idx] == 0) {
		particles[idx] = 0;
		particles[bottom_idx] = 1;
	} else if (temp_particles[bottom_left_idx] == 0) {
		particles[idx] = 0;
		particles[bottom_left_idx] = 1;
	} else if (temp_particles[bottom_right_idx] == 0) {
		particles[idx] = 0;
		particles[bottom_right_idx] = 1;
	}
}

void update_water(ivec2 coords, uint idx) {
	groupMemoryBarrier();

	ivec2 bottom_coords = ivec2(coords.x, coords.y + 1);
	ivec2 bottom_left_coords = ivec2(coords.x - 1, coords.y + 1);
	ivec2 bottom_right_coords = ivec2(coords.x + 1, coords.y + 1);
	ivec2 left_coords = ivec2(coords.x - 1, coords.y);
	ivec2 right_coords = ivec2(coords.x + 1, coords.y);

	if (bottom_coords.y >= config.sim_size.y) { return;	}
	if (bottom_left_coords.x < 0) { return;	}
	if (bottom_right_coords.x >= config.sim_size.x) {return; }
	if (right_coords.x >= config.sim_size.x) { return; }
	if (left_coords.x < 0) { return; }

	int bottom_idx = bottom_coords.x + bottom_coords.y * config.sim_size.x;
	int bottom_left_idx = bottom_left_coords.x + bottom_left_coords.y * config.sim_size.x;
	int bottom_right_idx = bottom_right_coords.x + bottom_right_coords.y * config.sim_size.x;
	int left_idx = left_coords.x + left_coords.y * config.sim_size.x;
	int right_idx = right_coords.x + right_coords.y * config.sim_size.x;

	if (temp_particles[bottom_idx] == 0) {
		particles[idx] = 0;
		particles[bottom_idx] = 2;
	} else if (temp_particles[bottom_left_idx] == 0) {
		particles[idx] = 0;
		particles[bottom_left_idx] = 2;
	} else if (temp_particles[bottom_right_idx] == 0) {
		particles[idx] = 0;
		particles[bottom_right_idx] = 2;
	} else if (temp_particles[right_idx] == 0) {
		particles[idx] = 0;
		particles[right_idx] = 2;
	} else if (temp_particles[left_idx] == 0) {
		particles[idx] = 0;
		particles[left_idx] = 2;
	}
}

void main() {
	if (gl_GlobalInvocationID.x >= config.sim_size.x || gl_GlobalInvocationID.y >= config.sim_size.y) { return; }

	uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * config.sim_size.x;
	ivec2 coords = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);

	int type = temp_particles[idx];

	vec4 col = vec4(0.0);
	if (type == 1) {
		update_sand(coords, idx);
	} else if (type == 2) {
		update_water(coords, idx);
	}

	type = particles[idx];
	if (type == 2) {
		col = vec4(0.1, 0.1, 1.0, 1.0);
	} else if (type == 1) {
		col = vec4(0.666, 0.576, 0.313, 1.0);
	}

	imageStore(result, coords, col);
}
