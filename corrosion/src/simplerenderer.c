#include "bir.h"
#include "simplerenderer.h"
#include "window.h"

static void create_pipeline(struct simple_renderer* renderer) {
	renderer->pipeline = video.new_pipeline(
		pipeline_flags_blend | pipeline_flags_dynamic_scissor | pipeline_flags_draw_tris,
		renderer->shader,
		renderer->framebuffer,
		(struct pipeline_attribute_bindings) {
			.bindings = (struct pipeline_attribute_binding[]) {
				{
					.attributes = (struct pipeline_attributes) {
						.attributes = (struct pipeline_attribute[]) {
							{
								.name     = "position",
								.location = 0,
								.offset   = offsetof(struct simple_renderer_vertex, position),
								.type     = pipeline_attribute_vec2
							},
							{
								.name     = "uv",
								.location = 1,
								.offset   = offsetof(struct simple_renderer_vertex, uv),
								.type     = pipeline_attribute_vec2
							},
							{
								.name     = "colour",
								.location = 2,
								.offset   = offsetof(struct simple_renderer_vertex, colour),
								.type     = pipeline_attribute_vec4
							},
							{
								.name     = "use_texture",
								.location = 3,
								.offset   = offsetof(struct simple_renderer_vertex, use_texture),
								.type     = pipeline_attribute_float
							}
						},
						.count = 4,
					},
					.stride = sizeof(struct simple_renderer_vertex),
					.rate = pipeline_attribute_rate_per_vertex,
					.binding = 0
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
								.uniform.size = sizeof(renderer->vertex_ub)
							}
						},
						{
							.name    = "atlas",
							.binding = 1,
							.stage   = pipeline_stage_fragment,
							.resource = {
								.type = pipeline_resource_texture,
								.texture = renderer->atlas->texture
							}
						}
					},
					.count = 2,
				}
			},
			.count = 1
		}
	);

}

static bool overlap_clip(const struct simple_renderer* renderer, v4f rect) {
	return
		rect.z > renderer->clip.x &&
		rect.w > renderer->clip.y &&
		rect.x < renderer->clip.x + renderer->clip.z &&
		rect.y < renderer->clip.y + renderer->clip.w;
}

static void draw_text_character(void* uptr, const struct texture* atlas, v2f position, v4f rect, v4f colour) {
	simple_renderer_push(uptr, &(struct simple_renderer_quad) {
		.position = position,
		.dimensions = make_v2f(rect.z, rect.w),
		.colour = colour,
		.rect = rect,
		.texture = atlas
	});
}

struct simple_renderer* new_simple_renderer(const struct framebuffer* framebuffer) {
	struct simple_renderer* renderer = core_calloc(1, sizeof(struct simple_renderer));

	renderer->text_renderer = (struct text_renderer) {
		.uptr = renderer,
		.draw_character = draw_text_character
	};

	renderer->atlas = new_atlas(texture_flags_filter_none);

	renderer->shader = video.new_shader(bir_simple_csh, bir_simple_csh_size);
	renderer->framebuffer = framebuffer;

	renderer->max = 800;

	renderer->vb = video.new_vertex_buffer(null,
		sizeof(struct simple_renderer_vertex) * renderer->max * simple_renderer_verts_per_quad,
		vertex_buffer_flags_dynamic | vertex_buffer_flags_transferable);

	usize index_count = renderer->max * simple_renderer_indices_per_quad;
	u16* indices = core_alloc(index_count * sizeof(u16));

	u16 offset = 0;
	for (usize i = 0; i < index_count; i += simple_renderer_indices_per_quad) {
		indices[i + 0] = offset + 3;
		indices[i + 1] = offset + 2;
		indices[i + 2] = offset + 1;
		indices[i + 3] = offset + 3;
		indices[i + 4] = offset + 1;
		indices[i + 5] = offset + 0;

		offset += 4;
	}

	renderer->ib = video.new_index_buffer(indices, index_count, index_buffer_flags_u16);
	core_free(indices);

	create_pipeline(renderer);

	return renderer;
}

void free_simple_renderer(struct simple_renderer* renderer) {
	free_atlas(renderer->atlas);

	video.free_shader(renderer->shader);

	video.free_vertex_buffer(renderer->vb);
	video.free_index_buffer(renderer->ib);

	video.free_pipeline(renderer->pipeline);

	core_free(renderer);
}

void simple_renderer_push(struct simple_renderer* renderer, const struct simple_renderer_quad* quad) {
	if (renderer->count >= renderer->max) {
		usize old_size = renderer->max;
		renderer->max *= 2;

		struct vertex_buffer* old_vb = renderer->vb;

		renderer->vb = video.new_vertex_buffer(null,
			sizeof(struct simple_renderer_vertex) * renderer->max * simple_renderer_verts_per_quad,
			vertex_buffer_flags_dynamic | vertex_buffer_flags_transferable);
		video.copy_vertex_buffer(renderer->vb, 0, old_vb, 0, old_size);
		video.free_vertex_buffer(old_vb);

		usize index_count = renderer->max * simple_renderer_indices_per_quad;
		u16* indices = core_alloc(index_count * sizeof(u16));

		u16 offset = 0;
		for (usize i = 0; i < index_count; i += simple_renderer_indices_per_quad) {
			indices[i + 0] = offset + 3;
			indices[i + 1] = offset + 2;
			indices[i + 2] = offset + 1;
			indices[i + 3] = offset + 3;
			indices[i + 4] = offset + 1;
			indices[i + 5] = offset + 0;

			offset += 4;
		}

		video.free_index_buffer(renderer->ib);
		renderer->ib = video.new_index_buffer(indices, index_count, index_buffer_flags_u16);
		core_free(indices);
	}

	f32 x1 = quad->position.x;
	f32 y1 = quad->position.y;
	f32 x2 = quad->position.x + quad->dimensions.x;
	f32 y2 = quad->position.y + quad->dimensions.y;

	if (!overlap_clip(renderer, make_v4f(x1, y1, x2, y2))) {
		return;
	}

	v4f rect = quad->rect;
	f32 tx = 0.0f, ty = 0.0f, tw = 0.0f, th = 0.0f;

	if (quad->texture) {
		v4i* atlas_rect = table_get(renderer->atlas->rects, quad->texture);
		if (!atlas_rect) {
			if (atlas_add_texture(renderer->atlas, quad->texture)) {
				video.free_pipeline(renderer->pipeline);
				create_pipeline(renderer);
				return;
			}

			atlas_rect = table_get(renderer->atlas->rects, quad->texture);
		}

		rect.x += (f32)atlas_rect->x;
		rect.y += (f32)atlas_rect->y;
		rect.z = cr_min((f32)atlas_rect->z, rect.z);
		rect.w = cr_min((f32)atlas_rect->w, rect.w);

		tx = rect.x / (f32)renderer->atlas->size.x;
		ty = rect.y / (f32)renderer->atlas->size.y;
		tw = rect.z / (f32)renderer->atlas->size.x;
		th = rect.w / (f32)renderer->atlas->size.y;
	}

	f32 use_texture = quad->texture != null ? 1.0f : 0.0f;
 
 	struct simple_renderer_vertex verts[] = {
 		{ .position = { x1, y2 }, .uv = { tx,      ty + th }, .colour = quad->colour, .use_texture = use_texture },
		{ .position = { x2, y2 }, .uv = { tx + tw, ty + th }, .colour = quad->colour, .use_texture = use_texture },
		{ .position = { x2, y1 }, .uv = { tx + tw, ty      }, .colour = quad->colour, .use_texture = use_texture },
		{ .position = { x1, y1 }, .uv = { tx,      ty      }, .colour = quad->colour, .use_texture = use_texture }
	};

	video.update_vertex_buffer(renderer->vb, verts, sizeof(verts),
		(renderer->offset + renderer->count) * sizeof(verts));

	renderer->count++;
}

void simple_renderer_flush(struct simple_renderer* renderer) {
	v2i window_size = video.get_framebuffer_size(renderer->framebuffer);

	renderer->vertex_ub.projection = m4f_ortho(0.0f, (f32)window_size.x, (f32)window_size.y, 0.0f, -1.0f, 1.0f);
	video.update_pipeline_uniform(renderer->pipeline, "primary", "VertexUniformData", &renderer->vertex_ub);

	video.begin_pipeline(renderer->pipeline);
		video.set_scissor(renderer->clip);

		video.bind_vertex_buffer(renderer->vb, 0);
		video.bind_index_buffer(renderer->ib);
		video.bind_pipeline_descriptor_set(renderer->pipeline, "primary", 0);
		video.draw_indexed(renderer->count * simple_renderer_indices_per_quad, renderer->offset * simple_renderer_indices_per_quad, 1);
	video.end_pipeline(renderer->pipeline);

	renderer->count = 0;
}

void simple_renderer_clip(struct simple_renderer* renderer, v4i clip) {
	if (renderer->count > 0) {
		usize count = renderer->count;
		simple_renderer_flush(renderer);
		renderer->offset += count;
	}

	renderer->clip = clip;
}

void simple_renderer_push_text(struct simple_renderer* renderer, const struct simple_renderer_text* text) {
	render_text(&renderer->text_renderer, text->font, text->text, text->position, text->colour);
}

void simple_renderer_end_frame(struct simple_renderer* renderer) {
	renderer->count = 0;
	renderer->offset = 0;
}
