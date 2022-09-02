#include "render_util.h"

struct aabb transform_aabb(const struct aabb* aabb, m4f m) {
	v3f corners[] = {
		aabb->min,
		make_v3f(aabb->min.x, aabb->max.y, aabb->min.z),
		make_v3f(aabb->min.x, aabb->max.y, aabb->max.z),
		make_v3f(aabb->min.x, aabb->min.y, aabb->max.z),
		make_v3f(aabb->max.x, aabb->min.y, aabb->min.z),
		make_v3f(aabb->max.x, aabb->max.y, aabb->min.z),
		aabb->max,
		make_v3f(aabb->max.x, aabb->min.y, aabb->max.z)
	};

	struct aabb r = {
		.min = { INFINITY, INFINITY, INFINITY },
		.max = { -INFINITY, -INFINITY, -INFINITY }
	};

	for (u32 i = 0; i < 8; i++) {
		v4f point = m4f_transform(m, make_v4f(corners[i].x, corners[i].y, corners[i].z, 1.0f));

		r.min.x = cr_min(r.min.x, point.x);
		r.min.y = cr_min(r.min.y, point.y);
		r.min.z = cr_min(r.min.z, point.z);
		r.max.x = cr_max(r.max.x, point.x);
		r.max.y = cr_max(r.max.y, point.y);
		r.max.z = cr_max(r.max.z, point.z);
	}

	return r;
}

force_inline struct frustum_plane make_plane(v3f normal, v3f pos) {
	v3f n_n = v3f_normalised(normal);
	return (struct frustum_plane) {
		.normal = n_n,
		.distance = v3_dot(n_n, pos)
	};
}

void compute_frustum_planes(m4f vp, struct frustum_plane* planes) {
	/* left */
	planes[0].normal.x = vp.m[0][3] + vp.m[0][0];
	planes[0].normal.y = vp.m[1][3] + vp.m[1][0];
	planes[0].normal.z = vp.m[2][3] + vp.m[2][0];
	planes[0].distance = vp.m[3][3] + vp.m[3][0];

	/* right */
	planes[1].normal.x = vp.m[0][3] - vp.m[0][0];
	planes[1].normal.y = vp.m[1][3] - vp.m[1][0];
	planes[1].normal.z = vp.m[2][3] - vp.m[2][0];
	planes[1].distance = vp.m[3][3] - vp.m[3][0];

	/* top */
	planes[2].normal.x = vp.m[0][3] - vp.m[0][1];
	planes[2].normal.y = vp.m[1][3] - vp.m[1][1];
	planes[2].normal.z = vp.m[2][3] - vp.m[2][1];
	planes[2].distance = vp.m[3][3] - vp.m[3][1];

	/* bottom */
	planes[3].normal.x = vp.m[0][3] + vp.m[0][1];
	planes[3].normal.y = vp.m[1][3] + vp.m[1][1];
	planes[3].normal.z = vp.m[2][3] + vp.m[2][1];
	planes[3].distance = vp.m[3][3] + vp.m[3][1];

	/* near */
	planes[4].normal.x = vp.m[0][3] + vp.m[0][2];
	planes[4].normal.y = vp.m[1][3] + vp.m[1][2];
	planes[4].normal.z = vp.m[2][3] + vp.m[2][2];
	planes[4].distance = vp.m[3][3] + vp.m[3][2];

	/* far */
	planes[5].normal.x = vp.m[0][3] - vp.m[0][2];
	planes[5].normal.y = vp.m[1][3] - vp.m[1][2];
	planes[5].normal.z = vp.m[2][3] - vp.m[2][2];
	planes[5].distance = vp.m[3][3] - vp.m[3][2];

	/* Normalise */
	for (usize i = 0; i < 6; i++) {
		f32 len = v3_mag(planes[i].normal);
		planes[i].normal.x /= len;
		planes[i].normal.y /= len;
		planes[i].normal.z /= len;
		planes[i].distance /= len;
	}
}

bool in_frustum(const struct aabb* aabb, const struct frustum_plane* planes) {
	const v3f extents = v3f_sub(aabb->max, aabb->min);
	const v3f pos = v3f_add(aabb->min, v3f_scale(extents, 0.5f));

	for (usize i = 0; i < 6; i++) {
		const struct frustum_plane* plane = planes + i;

		const f32 r = v3_dot(v3_abs(plane->normal), extents);

		if (v3_dot(plane->normal, pos) + plane->distance <= -r) {
			return false;
		}
	}

	return true;
}

struct texture* rgb_noise_texture(u32 flags, v2i size) {
	v4f* noise = core_calloc(size.x * size.y, sizeof *noise);

	for (i32 i = 0; i < size.x * size.y; i++) {
		noise[i] = make_v4f(
			random_float(-1.0f, 1.0f),
			random_float(-1.0f, 1.0f),
			random_float(-1.0f, 1.0f),
			random_float(-1.0f, 1.0f)
		);
	}

	struct image image = {
		.size = size,
		.colours = (void*)noise
	};

	struct texture* t = video.new_texture(&image, flags, texture_format_rgba32f);
	core_free(noise);
	return t;
}

struct texture* simplex_noise_texture(u32 flags, v2i size) {
	abort_with("Not implemented.");
	return null;
}
