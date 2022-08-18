#version 440

#compute

/* Compute shader that updates particles and then outputs their resulting positions */

layout (local_size_x = 256) in;

layout (binding = 0) uniform Config {
	uint particle_count;
	float ts;
	vec2 window_size;
} config;

struct ParticleInfo {
	vec4 colour;
};

struct Particle {
	vec2 pos;
	vec2 vel;
	vec4 colour;
};

layout (binding = 1) buffer Buffer {
	Particle particles[];
};

void main() {
	uint id = gl_GlobalInvocationID.x;
	if (id >= config.particle_count) { return; }

	vec2 pos = particles[id].pos;
	vec2 vel = particles[id].vel;

	if (pos.x < 0.0 || pos.x > config.window_size.x) {
		vel.x = -vel.x;
	}

	if (pos.y < 0.0 || pos.y > config.window_size.y) {
		vel.y = -vel.y;
	}

	pos += vel * config.ts;

	particles[id].pos = pos;
	particles[id].vel = vel;
}
