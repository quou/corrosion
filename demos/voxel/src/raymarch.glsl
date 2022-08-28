#version 440

#begin vertex

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out VSOut {
	vec2 uv;
} vs_out;

void main() {
	vs_out.uv = uv;

	gl_Position = vec4(position, 0.0, 1.0);
}

#end vertex

#begin fragment

#define max_ray_steps 256
#define max_ray_dist  256.0
#define min_ray_dist  0.001

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec2 uv;
} fs_in;

layout (binding = 0) uniform RenderData {
	vec2 resolution;
	float fov;
	vec3 camera_position;
	mat4 camera;
};

float get_sdf(vec3 p) {
	return min(
		length(max(abs(p) - vec3(0.5), 0.0)),
		length(max(abs(p + vec3(2.0, 0.0, 0.0)) - vec3(0.5), 0.0)));
}

float ray_march(vec3 origin, vec3 direction) {
	float dist = 0.0;
	for (int i = 0; i < max_ray_steps; i++) {
		vec3 p = origin + dist * direction;
		float hit = get_sdf(p);
		dist += hit;
		if (abs(hit) < min_ray_dist || dist > max_ray_dist) break;
	}
	return dist;
}

void render(inout vec3 col, vec2 pixel_coord) {
	vec3 origin = camera_position;
	vec3 direction = normalize(vec3(pixel_coord, fov));
	direction = (transpose(camera) * vec4(direction, 0.0)).xyz;

	float dist = ray_march(origin, direction);

	if (dist < max_ray_dist) {
		col = vec3(1.0);
	}
}

void main() {
	vec2 r = resolution;
	vec3 pixel_colour = vec3(0.0);

	vec2 pixel_coord = (2.0 * gl_FragCoord.xy - resolution.xy) / resolution.y;

	render(pixel_colour, pixel_coord);

	colour = vec4(pixel_colour, 1.0);
}

#end fragment
