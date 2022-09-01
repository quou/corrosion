#version 440

#compute


#define max_ray_steps 256
#define max_ray_dist  256.0
#define min_ray_dist  0.001

layout (local_size_x = 16, local_size_y = 16) in;

layout (binding = 0) uniform RenderData {
	ivec2 resolution;
	float fov;
	vec3 camera_position;
	mat4 camera;
};

layout (binding = 1, rgba16f) uniform image2D output_image;

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
	ivec2 coords = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	if (coords.x > resolution.x || coords.y > resolution.y) {
		return;
	}

	vec2 uv = (2.0 * coords - resolution) / resolution.y;

	vec3 colour = vec3(0.0);

	render(colour, uv);

	imageStore(output_image, coords, vec4(colour, 1.0));
}
