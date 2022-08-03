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
		.distance = v3f_dot(n_n, pos)
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

static bool in_front_of_plane(const struct aabb* aabb, const struct frustum_plane* plane) {
	const v3f extents = v3f_sub(aabb->max, aabb->min);
	const v3f centre = v3f_add(aabb->min, v3f_scale(extents, 0.5f));

	const f32 r =
		extents.x * fabsf(plane->normal.x) +
		extents.y * fabsf(plane->normal.y) +
		extents.z * fabsf(plane->normal.z);

    return -r <= v3f_dot(plane->normal, centre) - plane->distance;
}

bool in_frustum(const struct aabb* aabb, const struct frustum_plane* planes) {
	const v3f extents = v3f_sub(aabb->max, aabb->min);
	const v3f pos = v3f_add(aabb->min, v3f_scale(extents, 0.5f));

	for (usize i = 0; i < 6; i++) {
		const struct frustum_plane* plane = planes + i;

		const f32 r =
			extents.x * fabsf(plane->normal.x) +
			extents.y * fabsf(plane->normal.y) +
			extents.z * fabsf(plane->normal.z);

		if ((plane->normal.x * pos.x) + (plane->normal.y * pos.y) + (plane->normal.z * pos.z) + plane->distance <= -r) {
			return false;
		}
	}

	return true;
}
