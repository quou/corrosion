#include "bir.h"
#include "core.h"
#include "gizmo.h"

#define max_lines 25000

struct line_vertex {
	v3f position;
	v4f colour;
};

struct {
	usize vertex_count;

	v4f colour;

	struct vertex_buffer* vb;
	struct pipeline* pip;

	struct shader* shader;

	struct {
		m4f camera;
	} vertex_uniform_data;
} gizmos;

static void add_vertex(v3f position, v4f colour) {
	struct line_vertex v = {
		.position = position,
		.colour = colour
	};

	video.update_vertex_buffer(gizmos.vb, &v, sizeof v, gizmos.vertex_count * sizeof v);

	gizmos.vertex_count++;
}

void gizmos_init() {
	memset(&gizmos, 0, sizeof gizmos);

	gizmos.colour = make_rgba(0xffffff, 255);

	gizmos.vb = video.new_vertex_buffer(null, max_lines * sizeof(struct line_vertex), vertex_buffer_flags_dynamic);

	gizmos.shader = video.new_shader(bir_line_csh, bir_line_csh_size);

	gizmos.pip = video.new_pipeline(
		pipeline_flags_draw_lines,
		gizmos.shader,
		video.get_default_fb(),
		(struct pipeline_attribute_bindings) {
			.bindings = (struct pipeline_attribute_binding[]) {
				{
					.attributes = (struct pipeline_attributes) {
						.attributes = (struct pipeline_attribute[]) {
							{
								.name     = "position",
								.location = 0,
								.offset   = offsetof(struct line_vertex, position),
								.type     = pipeline_attribute_vec3
							},
							{
								.name     = "colour",
								.location = 1,
								.offset   = offsetof(struct line_vertex, colour),
								.type     = pipeline_attribute_vec4
							},
						},
						.count = 2,
					},
					.stride = sizeof (struct line_vertex),
					.rate = pipeline_attribute_rate_per_vertex,
					.binding = 0,
				}
			},
			.count = 1
		},
		(struct pipeline_descriptor_sets) {
			.sets = (struct pipeline_descriptor_set[]) {
				{
					.name = "primary",
					.descriptors = (struct pipeline_descriptor[]) {
						{
							.name     = "VertexUniformData",
							.binding  = 0,
							.stage    = pipeline_stage_vertex,
							.resource = {
								.type         = pipeline_resource_uniform_buffer,
								.uniform.size = sizeof gizmos.vertex_uniform_data
							}
						},
					},
					.count = 1,
				}
			},
			.count = 1
		}
	);
}

void gizmos_deinit() {
	video.free_shader(gizmos.shader);
	video.free_pipeline(gizmos.pip);
	video.free_vertex_buffer(gizmos.vb);
}

void gizmo_camera(const struct camera* camera) {
	gizmos.vertex_uniform_data.camera = m4f_mul(get_camera_projection(camera), get_camera_view(camera));
}

void gizmo_colour(v4f colour) {
	gizmos.colour = colour;
}

void gizmo_line(v3f begin, v3f end) {
	add_vertex(begin, gizmos.colour);
	add_vertex(end, gizmos.colour);
}

void gizmo_box(v3f centre, v3f dimensions, quat orientation) {
	v3f min_corner = make_v3f(-0.5f, -0.5f, -0.5f);
	v3f max_corner = make_v3f( 0.5f,  0.5f,  0.5f);

	v3f corners[] = {
		min_corner,
		make_v3f(min_corner.x, max_corner.y, min_corner.z),
		make_v3f(min_corner.x, max_corner.y, max_corner.z),
		make_v3f(min_corner.x, min_corner.y, max_corner.z),
		make_v3f(max_corner.x, min_corner.y, min_corner.z),
		make_v3f(max_corner.x, max_corner.y, min_corner.z),
		max_corner,
		make_v3f(max_corner.x, min_corner.y, max_corner.z)
	};

	m4f transform = m4f_mul(m4f_mul(m4f_translation(centre), m4f_rotation(orientation)), m4f_scale(dimensions));

	for (usize i = 0; i < 8; i++) {
		v3f corner = corners[i];

		v4f t_c = m4f_transform(transform, make_v4f(corner.x, corner.y, corner.z, 1.0f));
		corners[i] = make_v3f(t_c.x, t_c.y, t_c.z);
		corner = corners[i];
	}

	gizmo_line(corners[0], corners[1]);
	gizmo_line(corners[1], corners[2]);
	gizmo_line(corners[2], corners[3]);
	gizmo_line(corners[3], corners[0]);
	gizmo_line(corners[4], corners[5]);
	gizmo_line(corners[5], corners[6]);
	gizmo_line(corners[6], corners[7]);
	gizmo_line(corners[7], corners[4]);
	gizmo_line(corners[0], corners[4]);
	gizmo_line(corners[1], corners[5]);
	gizmo_line(corners[2], corners[6]);
	gizmo_line(corners[3], corners[7]);
}

void gizmo_sphere(v3f centre, f32 radius) {
}

void gizmos_draw() {
	video.update_pipeline_uniform(gizmos.pip, "primary", "VertexUniformData", &gizmos.vertex_uniform_data);

	if (gizmos.vertex_count > 0) {
		video.begin_pipeline(gizmos.pip);
			video.bind_vertex_buffer(gizmos.vb, 0);
			video.bind_pipeline_descriptor_set(gizmos.pip, "primary", 0);
			video.draw(gizmos.vertex_count, 0, 1);
		video.end_pipeline(gizmos.pip);

		gizmos.vertex_count = 0;
	}
}
