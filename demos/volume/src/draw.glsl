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

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec2 uv;
} fs_in;

layout (binding = 0) uniform sampler3D volume;

layout (binding = 1) uniform Config {
	float time;
	float threshold;
	vec2 resolution;
	mat4 view;
	vec3 camera_position;
};

/* From Sebastian Lague's cloud rendering video. */
vec2 ray_vs_box(vec3 bmin, vec3 bmax, vec3 origin, vec3 dir) {
	vec3 t0 = (bmin - origin) / dir;
	vec3 t1 = (bmax - origin) / dir;
	vec3 tmin = min(t0, t1);
	vec3 tmax = max(t0, t1);

	float dst_a = max(max(tmin.x, tmin.y), tmin.z);
	float dst_b = min(tmax.x, min(tmax.y, tmax.z));

	float dst_to_box = max(0.0, dst_a);
	float dst_inside_box = max(0.0, dst_b - dst_to_box);
	return vec2(dst_to_box, dst_inside_box);
}

void main() {
	const float fov = 1.0;
	const int sample_count = 128;

	vec2 uv = (2.0 * gl_FragCoord.xy - resolution) / resolution.y;

	vec3 ray_origin = camera_position;
	vec3 ray_dir = (transpose(view) * vec4(normalize(vec3(uv, fov)), 0.0)).xyz;

	vec3 box_pos = vec3(0.0, 0.0, -3.0);
	vec3 box_size = vec3(10.0, 10.0, 10.0);
	vec2 r = ray_vs_box(box_pos + box_size * 0.5, box_pos - box_size * 0.5, ray_origin, ray_dir);

	float cloud_scale = 0.1;

	/* Sample the first layer of the volume texture to use
	 * as 2D noise for the background. */
	float bnoise = texture(volume, vec3(fs_in.uv + time * 0.01, 0.0)).r;

	vec4 background_colour = vec4(0.0);

	if (bnoise > 0.1) {
		background_colour = vec4(0.4, 0.0, 0.0, 1.0);
	} else if (bnoise > 0.09) {
		background_colour = vec4(0.2, 0.0, 0.0, 1.0);
	} else if (bnoise > 0.07) {
		background_colour = vec4(0.0, 0.0, 0.1, 1.0);
	}

	if (r.y > 0.0) {
		float step_size = r.y / float(sample_count);

		float density = 0.0;
		float cur = 0.0;

		for (int i = 0; i < sample_count; i++) {
			vec3 p = ray_origin + ray_dir * (r.x + cur);

			density += max(0.0, (1.0 - texture(volume, (p * 0.5 * cloud_scale) + time * 0.03).r) - threshold);

			cur += step_size;
		}

		float t = exp(-density);

		vec4 cloud_colour = vec4(1.0);

		colour = mix(cloud_colour, background_colour, t);
	} else {
		colour = background_colour;
	}
}

#end fragment
