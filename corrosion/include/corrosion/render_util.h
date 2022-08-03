#pragma once

#include "video.h"

struct aabb {
	v3f min;
	v3f max;
};

struct aabb transform_aabb(const struct aabb* aabb, m4f m);

struct frustum_plane {
	f32 distance;
	v3f normal;
};

/* Calculates planes that form to the frustum of a camera. Used for frustum culling.
 * `planes' should be an array of at least six elements.
 *
 * The resulting frustum planes are organised in the following order within the array:
 *     left
 *     right
 *     top
 *     bottom
 *     near
 *     far
 */
void compute_frustum_planes(m4f vp, struct frustum_plane* planes);

/* Checks if an AABB is inside a frustum, for frustum culling. `planes` should be
 * an array of at least six elements. */
bool in_frustum(const struct aabb* aabb, const struct frustum_plane* planes);
