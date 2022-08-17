#pragma once

#include "common.h"
#include "maths.h"
#include "video.h"

void gizmos_init();
void gizmos_deinit();

/* TODO: Movement gizmos.
 * TODO: Sphere gizmo.
 * TODO: Sprite gizmo.
 */

void gizmo_camera(const struct camera* camera);
void gizmo_colour(v4f colour);
void gizmo_line(v3f begin, v3f end);
void gizmo_box(v3f centre, v3f dimensions, quat orientation);
void gizmo_sphere(v3f centre, f32 radius);
void gizmos_draw();
