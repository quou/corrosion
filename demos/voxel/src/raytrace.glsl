#version 440

#compute

#define max_ray_steps 256
#define max_ray_dist  256.0
#define min_ray_dist  0.001
#define voxel_size 2.0

const float infinity = 1.0 / 0.0;

layout (local_size_x = 16, local_size_y = 16) in;

struct Light {
	vec3 position;
	float power;
	vec3 colour;
};

layout (binding = 3) uniform RenderData {
	ivec2 resolution;
	float fov;
	vec3 camera_position;
	mat4 view;
	vec3 chunk_pos;
	vec3 chunk_extent;

	uint light_count;
	Light lights[32];
};

layout (binding = 0, rgba16f) uniform image2D output_colour;
layout (binding = 1, r32f)    uniform image2D output_depth;
layout (binding = 2) buffer VoxelBuffer {
	uint voxels[];
};

struct Ray {
	vec3 origin;
	vec3 dir;
};

struct Bound {
	vec3 mini;
	vec3 maxi;
};

bool voxel_at(in vec3 pos, out vec3 colour) {
	uvec3 e = uvec3(uint(chunk_extent.x), uint(chunk_extent.y), uint(chunk_extent.z));
	uvec3 p = uvec3(uint(pos.x), uint(pos.y), uint(pos.z));

	if (p.x >= e.x || p.y >= e.y || p.z >= e.z) {
		return false;
	}

	uint v = voxels[(e.x * e.y * p.z) + (e.y * p.y) + p.x];

	colour.r = (float(((v >> 24) & 0xff))) / 255.0;
	colour.g = (float(((v >> 16) & 0xff))) / 255.0;
	colour.b = (float(((v >> 8)  & 0xff))) / 255.0;

	return v != 0.0;
}

float get_voxel(in vec3 pos) {
	uvec3 e = uvec3(uint(chunk_extent.x), uint(chunk_extent.y), uint(chunk_extent.z));
	uvec3 p = uvec3(uint(pos.x), uint(pos.y), uint(pos.z));

	if (p.x >= e.x || p.y >= e.y || p.z >= e.z) {
		return float(0.0);
	}

	uint v = voxels[(e.x * e.y * p.z) + (e.y * p.y) + p.x];

	return float(v != 0.0);
}

bool intersect_bound(in Ray ray, in Bound bound, out float t_near, out float t_far) {
	float t0 = 0.0, t1 = max_ray_dist;
	for (int i = 0; i < 3; i++) {
		float inv_ray_dir = 1.0 / ray.dir[i];
		float near = (bound.mini[i] - ray.origin[i]) * inv_ray_dir;
		float far  = (bound.maxi[i] - ray.origin[i]) * inv_ray_dir;

		if (near > far) {
			float temp = near;
			near = far;
			far = temp;
		}

		t0 = near > t0 ? near : t0;
		t1 = far  < t1 ? far  : t1;

		if (t0 > t1) { return false; }
	}

	t_near = t0;
	t_far = t1;
	return true;
}

bool point_inside_bound(in vec3 pos, in Bound bound) {
	bool inside = true;

	for (int i = 0; i < 3; i++) {
		if (pos[i] < bound.mini[i]) {
			inside = false;
		}

		if (pos[i] > bound.maxi[i]) {
			inside = false;
		}
	}

	return inside;
}

/* From https://github.com/fenomas/fast-voxel-raycast/blob/master/index.js */
bool raytrace(in Ray ray, in Bound bound, float max_dist, out vec3 colour, out vec3 pos, out vec3 normal) {
	float t = 0.0;
	vec3 i = floor(ray.origin);

	vec3 step = sign(ray.dir);

	vec3 delta_t = abs(1.0 / ray.dir);

	vec3 dist;
	dist.x = (step.x > 0) ? (i.x + 1 - ray.origin.x) : (ray.origin.x - i.x);
	dist.y = (step.y > 0) ? (i.y + 1 - ray.origin.y) : (ray.origin.y - i.y);
	dist.z = (step.z > 0) ? (i.z + 1 - ray.origin.z) : (ray.origin.z - i.z);

	int stepped_index = -1;

	vec3 max_t;
	max_t.x = (delta_t.x < infinity) ? (delta_t.x * dist.x) : (infinity);
	max_t.y = (delta_t.y < infinity) ? (delta_t.y * dist.y) : (infinity);
	max_t.z = (delta_t.z < infinity) ? (delta_t.z * dist.z) : (infinity);

	/* TODO: Do it without branches */

	while (t < max_dist) {
		if (!point_inside_bound(i, bound)) {
			return false;
		}

		if (voxel_at(i, colour)) {
			pos = ray.origin + t * ray.dir;

			normal = vec3(0.0);
			
			switch (stepped_index) {
				case 0: normal.x = -step.x; break;
				case 1: normal.y = -step.y; break;
				case 2: normal.z = -step.z; break;
			}

			return true;
		}

		if (max_t.x < max_t.y) {
			if (max_t.x < max_t.z) {
				i.x += step.x;
				t = max_t.x;
				max_t.x += delta_t.x;
				stepped_index = 0;
			} else {
				i.z += step.z;
				t = max_t.z;
				max_t.z += delta_t.z;
				stepped_index = 2;
			}
		} else {
			if (max_t.y < max_t.z) {
				i.y += step.y;
				t = max_t.y;
				max_t.y += delta_t.y;
				stepped_index = 1;
			} else {
				i.z += step.z;
				t = max_t.z;
				max_t.z += delta_t.z;
				stepped_index = 2;
			}
		}
	}

	return false;
}

void main() {
	ivec2 coords = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	if (coords.x > resolution.x || coords.y > resolution.y) {
		return;
	}

	vec2 uv = (2.0 * coords - resolution) / resolution.y;

	vec3 colour = vec3(0.0);
	vec3 normal = vec3(0.0);
	vec3 position = vec3(0.0);

	Ray primary = Ray(
		camera_position,
		(transpose(view) * vec4(normalize(vec3(uv, fov)), 0.0)).xyz
	);

	Bound chunk_bound = Bound(
		chunk_pos,
		chunk_pos + chunk_extent
	);

	float t_near, t_far;
	bool intersect = intersect_bound(primary, chunk_bound, t_near, t_far);

	vec3 sun_dir = vec3(0.3, 0.3, 0.3);

	if (intersect) {
		Ray secondary = Ray(
			primary.origin + primary.dir * t_near + 0.001,
			primary.dir
		);

		vec3 vcol, vnorm, vpos;
		if (raytrace(secondary, chunk_bound, 1000.0, vcol, vpos, vnorm)) {
			normal = vnorm;
			position = (view * vec4(vpos, 1.0)).xyz;

			vec3 view_dir = normalize(camera_position - vpos);

			/* Apply lighting. */
			vec3 lighting = vcol * vec3(0.1);
			for (uint i = 0; i < light_count; i++) {
				Light light = lights[i];
				Ray light_ray = Ray(
					vpos - secondary.dir * 0.001,
					normalize(light.position - vpos)
				);

				vec3 hcol, hnorm, hpos;
				float dist = length(light.position - vpos);
				if (!raytrace(light_ray, chunk_bound, dist, hcol, hnorm, hpos)) {
					float attenuation = 1.0 / (pow(dist, 2.0) + 1);

					dist = length(light.position - vpos);
					vec3 light_dir = normalize(light.position - vpos);
					vec3 reflect_dir = reflect(-light_dir, normal);

					lighting +=
						((vcol * vec3(0.1)) + vcol) *
						(
							attenuation *
							light.power *
							light.colour *
							max(dot(light_dir, normal), 0.0) +

							attenuation *
							light.power *
							light.colour *
							pow(max(dot(view_dir, reflect_dir), 0.0), 8.0)
						);
				}
			}

			colour = lighting * vcol;
		}
	}

	imageStore(output_colour, coords, vec4(colour, 1.0));
	imageStore(output_depth,  coords, vec4(position.z));
}
