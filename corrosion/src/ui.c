#include <stdio.h>
#include <ctype.h>

#include "bir.h"
#include "core.h"
#include "dtable.h"
#include "ui.h"
#include "ui_render.h"

enum {
	ui_cmd_draw_rect,
	ui_cmd_draw_outline,
	ui_cmd_draw_gradient,
	ui_cmd_draw_text,
	ui_cmd_draw_texture,
	ui_cmd_draw_circle,
	ui_cmd_clip
};

struct ui_cmd {
	u8 type;
	usize size;
};

struct ui_cmd_draw_rect {
	struct ui_cmd cmd;
	v2f position;
	v2f dimensions;
	v4f colour;
	f32 radius;
};

struct ui_cmd_draw_outline {
	struct ui_cmd cmd;
	v2f position;
	v2f dimensions;
	v4f colour;
	f32 radius;
	f32 thickness;
};

struct ui_cmd_draw_gradient {
	struct ui_cmd cmd;
	v2f position;
	v2f dimensions;
	v4f top_left;
	v4f top_right;
	v4f bot_left;
	v4f bot_right;
	f32 radius;
};

struct ui_cmd_draw_circle {
	struct ui_cmd cmd;
	v2f position;
	v4f colour;
	f32 radius;
};

struct ui_cmd_draw_text {
	struct ui_cmd cmd;
	struct font* font;
	v2f position;
	v2f dimensions;
	v4f colour;
};

struct ui_cmd_clip {
	struct ui_cmd cmd;
	v4i rect;
};

struct ui_cmd_texture {
	struct ui_cmd cmd;
	v4i rect;
	v2f position;
	v2f dimensions;
	v4f colour;
	const struct texture* texture;
	f32 radius;
	f32 z;
};

struct ui_container {
	v4f rect;
	v4f padding;
	f32 spacing;
	
	bool floating;
	v2f old_cursor_pos;

	u64 id;
	bool scrollable;
	bool interactable;

	i32 z;

	v2f content_size;
	f32 left_bound;
};

struct ui_cmd_view {
	usize head;
	usize tail;
};

struct ui_container_meta {
	v2f position;
	v2f dimensions;

	v2f scroll;
	i32 z;

	bool floating;
	bool visible;

	bool interactable;

	i32 life;

	struct ui_cmd_view* current_view;
	vector(struct ui_cmd_view) cmd_views;
};

enum {
	ui_align_left = 0,
	ui_align_right,
	ui_align_centre
};

enum {
	ui_style_variant_none = 0,
	ui_style_variant_hovered,
	ui_style_variant_active,
};

struct ui_style {
	optional(v4f) text_colour;
	optional(v4f) background_colour;
	optional(v4f) select_colour;
	optional(u32) align;
	optional(f32) radius;
	optional(v4f) padding;
	optional(v2f) max_size;
	optional(v2f) min_size;
	optional(f32) spacing;
	optional(f32) outline_thickness;
	optional(v4f) outline_colour;
};

struct ui_stylesheet {
	table(const char*, struct ui_style) normal;
	table(const char*, struct ui_style) hovered;
	table(const char*, struct ui_style) active;
} default_stylesheet;

struct container_commander {
	i32 z;
	usize offset;
};

struct font* default_font;

struct ui {
	struct ui_renderer* renderer;

	struct texture* alpha_texture;

	struct font* default_font;
	struct font* font;

	u8* cmd_buffer;
	usize cmd_buffer_idx;
	usize cmd_buffer_capacity;
	usize last_cmd_size;

	struct ui_stylesheet* stylesheet;

	v2f cursor_pos;
	v4f current_clip;

	char* wrap_buffer;
	usize wrap_buffer_size;

	vector(struct ui_container) container_stack;

	char* temp_str;
	usize temp_str_size;

	u64 active;
	u64 dragging;
	u64 hovered;

	u64 last_input_id;
	char* last_input_buf;

	i32 current_z;

	vector(void*) cmd_free_queue;

	u32 input_cursor;
	u32 input_select_start;

	f32 knob_angle_offset;

	u64 treenode_id;
	table(u64, bool) open_treenodes;

	u64 container_id;
	table(u64, struct ui_container_meta) container_meta;

	vector(struct ui_container_meta*) sorted_containers;

	/* Probably not the best way to solve this problem. */
	table(u64, bool) number_input_trailing_fullstops;

	usize column_count;
	usize column;
	f32 current_item_height;
	vector(f32) columns;
};

static inline bool mouse_over_rect(v2f position, v2f dimensions) {
	v2i mouse_pos = get_mouse_pos();

	return
		mouse_pos.x > (i32)position.x &&
		mouse_pos.y > (i32)position.y &&
		mouse_pos.x < (i32)position.x + dimensions.x &&
		mouse_pos.y < (i32)position.y + dimensions.y;
}

static bool rect_outside_clip(v2f position, v2f dimensions, v4i clip) {
	return
		position.x + dimensions.x > (f32)clip.x + (f32)clip.z ||
		position.x                < (f32)clip.x               ||
		position.y + dimensions.y > (f32)clip.y + (f32)clip.w ||
		position.y                < (f32)clip.y;
}

static v2f get_container_max_scroll(const struct ui_container* container, const struct ui_container_meta* meta) {
	return make_v2f(
		container->content_size.x - container->rect.z - meta->scroll.x,
		container->content_size.y - container->rect.w + container->padding.y);
}

/* Position is the top left of a bounding box around the circle. */
static inline bool mouse_over_circle(v2f position, f32 radius) {
	v2i mouse_pos = get_mouse_pos();

	return v2_mag_sqrd(v2f_sub(make_v2f(mouse_pos.x, mouse_pos.y), v2f_add(position, make_v2f(radius, radius)))) < radius * radius;
}

static void* ui_last_cmd(struct ui* ui) {
	return (ui->cmd_buffer + ui->cmd_buffer_idx) - ui->last_cmd_size;
}

static void* ui_cmd_add(struct ui* ui, usize size) {
	if (ui->cmd_buffer_idx + size >= ui->cmd_buffer_capacity) {
		usize old_cap = ui->cmd_buffer_capacity;
		ui->cmd_buffer_capacity += 1024;
		vector_push(ui->cmd_free_queue, ui->cmd_buffer);
		void* old = ui->cmd_buffer;
		ui->cmd_buffer = core_alloc(ui->cmd_buffer_capacity);
		memcpy(ui->cmd_buffer, old, old_cap);
	}

	ui->last_cmd_size = size;
	ui->cmd_buffer_idx += size;

	return ui->cmd_buffer + (ui->cmd_buffer_idx - size);
}

static void push_back_other_containers(struct ui* ui, const struct ui_container_meta* self) {
	for (u64* i = table_first(ui->container_meta); i; i = table_next(ui->container_meta, *i)) {
		struct ui_container_meta* m = table_get(ui->container_meta, *i);

		if (m != self) { m->z--; }
	}
}

void ui_draw_rect(struct ui* ui, v2f position, v2f dimensions, v4f colour, f32 radius) {
	struct ui_cmd_draw_rect* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_rect));
	cmd->cmd.type = ui_cmd_draw_rect;
	cmd->cmd.size = sizeof *cmd;
	cmd->position = position;
	cmd->dimensions = dimensions;
	cmd->colour = colour;
	cmd->radius = radius;
}

void ui_draw_outline(struct ui* ui, v2f position, v2f dimensions, v4f colour, f32 radius, f32 thickness) {
	struct ui_cmd_draw_outline* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_outline));
	cmd->cmd.type = ui_cmd_draw_outline;
	cmd->cmd.size = sizeof *cmd;
	cmd->position = position;
	cmd->dimensions = dimensions;
	cmd->colour = colour;
	cmd->radius = radius;
	cmd->thickness = thickness;
}

void ui_draw_gradient(struct ui* ui, v2f position, v2f dimensions, v4f top_left, v4f top_right, v4f bot_left, v4f bot_right, f32 radius) {
	struct ui_cmd_draw_gradient* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_gradient));
	cmd->cmd.type = ui_cmd_draw_gradient;
	cmd->cmd.size = sizeof *cmd;
	cmd->position = position;
	cmd->dimensions = dimensions;
	cmd->top_left = top_left;
	cmd->top_right = top_right;
	cmd->bot_left = bot_left;
	cmd->bot_right = bot_right;
	cmd->radius = radius;
}

void ui_draw_circle(struct ui* ui, v2f position, f32 radius, v4f colour) {
	struct ui_cmd_draw_circle* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_circle));
	cmd->cmd.type = ui_cmd_draw_circle;
	cmd->cmd.size = sizeof *cmd;
	cmd->position = position;
	cmd->colour = colour;
	cmd->radius = radius;
}

void ui_draw_text(struct ui* ui, v2f position, v2f dimensions, const char* text, v4f colour) {
	usize len = strlen(text);

	struct ui_cmd_draw_text* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_text) + len + 1);
	cmd->cmd.type = ui_cmd_draw_text;
	cmd->cmd.size = (sizeof *cmd) + len + 1;
	cmd->position = position;
	cmd->dimensions = dimensions;
	cmd->colour = colour;
	cmd->font = ui->font;

	memcpy(cmd + 1, text, len);
	((char*)(cmd + 1))[len] = '\0';
}

void ui_clip(struct ui* ui, v4f rect) {
	struct ui_cmd_clip* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_clip));
	cmd->cmd.type = ui_cmd_clip;
	cmd->cmd.size = sizeof *cmd;
	cmd->rect = make_v4i((f32)rect.x, (f32)rect.y, (f32)rect.z, (f32)rect.w);

	ui->current_clip = rect;
}

void ui_draw_texture(struct ui* ui, v2f position, v2f dimensions, const struct texture* texture, v4i rect, v4f colour, f32 radius) {
	struct ui_cmd_texture* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_texture));
	cmd->cmd.type = ui_cmd_draw_texture;
	cmd->cmd.size = sizeof *cmd;
	cmd->rect = rect;
	cmd->position = position;
	cmd->dimensions = dimensions;
	cmd->texture = texture;
	cmd->colour = colour;
	cmd->radius = radius;
}

v2f ui_get_cursor_pos(const struct ui* ui) {
	return ui->cursor_pos;
}

void ui_set_cursor_pos(struct ui* ui, v2f pos) {
	ui->cursor_pos = pos;
}

static inline void ui_build_style(struct ui_style* dst, const struct ui_style* src) {
	if (src->text_colour.has_value)       { dst->text_colour       = src->text_colour; }
	if (src->background_colour.has_value) { dst->background_colour = src->background_colour; }
	if (src->align.has_value)             { dst->align             = src->align; }
	if (src->padding.has_value)           { dst->padding           = src->padding; }
	if (src->radius.has_value)            { dst->radius            = src->radius; }
	if (src->min_size.has_value)          { dst->min_size          = src->min_size; }
	if (src->max_size.has_value)          { dst->max_size          = src->max_size; }
	if (src->outline_thickness.has_value) { dst->outline_thickness = src->outline_thickness; }
	if (src->outline_colour.has_value)    { dst->outline_colour    = src->outline_colour; }
}

static inline void ui_build_style_variant(struct ui* ui, const char* class_name, struct ui_style* dst, u32 variant) {
	switch (variant) {
		case ui_style_variant_hovered: {
			const struct ui_style* hovered_ptr = table_get(ui->stylesheet->hovered, class_name);

			if (hovered_ptr) {
				ui_build_style(dst, hovered_ptr);
			}
		} break;
		case ui_style_variant_active: {
			const struct ui_style* active_ptr = table_get(ui->stylesheet->active, class_name);

			if (active_ptr) {
				ui_build_style(dst, active_ptr);
			}
		} break;
		default: break;
	}
}

static struct ui_style ui_get_style(struct ui* ui, const char* base_class, const char* class, u32 variant) {
	const struct ui_style* base_ptr = table_get(ui->stylesheet->normal, base_class);

	if (!base_ptr) {
		error("Base class `%s' not found in stylesheet.", base_class);
		return (struct ui_style) { 0 };
	}

	struct ui_style base = *base_ptr;
	ui_build_style_variant(ui, base_class, &base, variant);

	usize class_name_size = strlen(class) + 1;
	if (class_name_size > ui->temp_str_size) {
		ui->temp_str = core_realloc(ui->temp_str, class_name_size);
		ui->temp_str_size = class_name_size;
	}

	memcpy(ui->temp_str, class, class_name_size);

	char* cur_class = strtok(ui->temp_str, " ");
	while (cur_class) {
		const struct ui_style* class_ptr = table_get(ui->stylesheet->normal, cur_class);

		if (!class_ptr) {
			error("Class `%s' not found in stylesheet.", cur_class);
		} else {
			ui_build_style(&base, class_ptr);
			ui_build_style_variant(ui, cur_class, &base, variant);
		}

		cur_class = strtok(null, " ");
	}

	f32 scale = get_dpi_scale();
	base.padding.value = v4f_scale(base.padding.value, scale);
	base.outline_thickness.value *= scale;

	return base;
}

static bool ui_get_style_variant(struct ui* ui, struct ui_style* style, const char* base, const char* class, bool hovered, bool active) {
	if (active) {
		*style = ui_get_style(ui, base, class, ui_style_variant_active);
		return true;
	} else if (hovered) {
		if (mouse_btn_pressed(mouse_btn_left)) {
			*style = ui_get_style(ui, base, class, ui_style_variant_active);
			return true;
		} else {
			*style = ui_get_style(ui, base, class, ui_style_variant_hovered);
			return true;
		}
	}

	return false;
}

static void stylesheet_table_from_dtable(void* dst_vptr, struct dtable* src) {
	table(const char*, struct ui_style)* dst = dst_vptr;

	for (usize i = 0; i < vector_count(src->children); i++) {
		struct dtable* class = src->children + i;

		struct ui_style s = { 0 };
		struct ui_style* got = table_get(*dst, class->key.chars);
		if (got) { s = *got; }

		for (usize j = 0; j < vector_count(class->children); j++) {
			struct dtable* child = class->children + j;

			switch (child->value.type) {
				case dtable_colour:
					if (strcmp(child->key.chars, "text_colour") == 0) {
						optional_set(s.text_colour, child->value.as.colour);
					} else if (strcmp(child->key.chars, "background_colour") == 0) {
						optional_set(s.background_colour, child->value.as.colour);
					} else if (strcmp(child->key.chars, "selection_colour") == 0) {
						optional_set(s.select_colour, child->value.as.colour);
					} else if (strcmp(child->key.chars, "outline_colour") == 0) {
						optional_set(s.outline_colour, child->value.as.colour);
					}
					break;
				case dtable_number: 
					if (strcmp(child->key.chars, "radius") == 0) {
						optional_set(s.radius, (f32)child->value.as.number);
					} else if (strcmp(child->key.chars, "outline_thickness") == 0) {
						optional_set(s.outline_thickness, (f32)child->value.as.number);
					}
					break;
				case dtable_string:
					if (strcmp(child->key.chars, "align") == 0) {
						optional_set(s.align, strcmp(child->value.as.string, "left") == 0 ? ui_align_left :
							strcmp(child->value.as.string, "centre") == 0 ? ui_align_centre :
							ui_align_right);
					}
					break;
				case dtable_v2:
					if (strcmp(child->key.chars, "max_size") == 0) {
						optional_set(s.max_size, child->value.as.v2);
					} else if (strcmp(child->key.chars, "min_size") == 0) {
						optional_set(s.min_size, child->value.as.v2);
					}
					break;
				case dtable_v4:
					if (strcmp(child->key.chars, "padding") == 0) {	
						optional_set(s.padding, child->value.as.v4);
					}
					break;
			}
		}

		table_set(*dst, class->key.chars, s);
	}
}

static void stylesheet_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	struct ui_stylesheet* stylesheet = payload;

	stylesheet->normal.hash     = table_hash_string;
	stylesheet->normal.compare  = table_compare_string;
	stylesheet->normal.free_key = table_free_string;
	stylesheet->normal.copy_key = table_copy_string;

	stylesheet->hovered.hash     = table_hash_string;
	stylesheet->hovered.compare  = table_compare_string;
	stylesheet->hovered.free_key = table_free_string;
	stylesheet->hovered.copy_key = table_copy_string;

	stylesheet->active.hash     = table_hash_string;
	stylesheet->active.compare  = table_compare_string;
	stylesheet->active.free_key = table_free_string;
	stylesheet->active.copy_key = table_copy_string;

	for (const char** i = table_first(default_stylesheet.normal); i; i = table_next(default_stylesheet.normal, *i)) {
		table_set(stylesheet->normal, *i, *(struct ui_style*)table_get(default_stylesheet.normal, *i));
	}

	for (const char** i = table_first(default_stylesheet.active); i; i = table_next(default_stylesheet.active, *i)) {
		table_set(stylesheet->active, *i, *(struct ui_style*)table_get(default_stylesheet.active, *i));
	}

	for (const char** i = table_first(default_stylesheet.hovered); i; i = table_next(default_stylesheet.hovered, *i)) {
		table_set(stylesheet->hovered, *i, *(struct ui_style*)table_get(default_stylesheet.hovered, *i));
	}

	struct dtable dt = { 0 };
	parse_dtable(&dt, (const char*)raw);

	struct dtable tab = { 0 };
	if (dtable_find_child(&dt, "normal", &tab)) {
		stylesheet_table_from_dtable(&stylesheet->normal, &tab);
	}

	if (dtable_find_child(&dt, "active", &tab)) {
		stylesheet_table_from_dtable(&stylesheet->active, &tab);
	}

	if (dtable_find_child(&dt, "hovered", &tab)) {
		stylesheet_table_from_dtable(&stylesheet->hovered, &tab);
	}

	deinit_dtable(&dt);
}

static void stylesheet_on_unload(void* payload, usize payload_size) {
	struct ui_stylesheet* stylesheet = payload;

	free_table(stylesheet->normal);
	free_table(stylesheet->active);
	free_table(stylesheet->hovered);
}

void ui_init() {
	memset(&default_stylesheet.normal,  0, sizeof default_stylesheet.normal);
	memset(&default_stylesheet.active,  0, sizeof default_stylesheet.active);
	memset(&default_stylesheet.hovered, 0, sizeof default_stylesheet.hovered);

	default_stylesheet.normal.hash     = table_hash_string;
	default_stylesheet.normal.compare  = table_compare_string;
	default_stylesheet.normal.free_key = table_free_string;
	default_stylesheet.normal.copy_key = table_copy_string;

	default_stylesheet.active.hash     = table_hash_string;
	default_stylesheet.active.compare  = table_compare_string;
	default_stylesheet.active.free_key = table_free_string;
	default_stylesheet.active.copy_key = table_copy_string;

	default_stylesheet.hovered.hash     = table_hash_string;
	default_stylesheet.hovered.compare  = table_compare_string;
	default_stylesheet.hovered.free_key = table_free_string;
	default_stylesheet.hovered.copy_key = table_copy_string;

	table_set(default_stylesheet.normal, "label", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x111111, 0) },
		.align             = { true, ui_align_left },
	}));

	table_set(default_stylesheet.normal, "button", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, make_v4f(3.0f, 3.0f, 3.0f, 3.0f) },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.hovered, "button", ((struct ui_style) {
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, "button", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, "input", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, make_v4f(3.0f, 3.0f, 3.0f, 3.0f) },
		.select_colour     = { true, make_rgba(0x191b26, 128) },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.hovered, "input", ((struct ui_style) {
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, "input", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, "container", ((struct ui_style) {
		.background_colour = { true, make_rgba(0x111111, 255) },
		.padding           = { true, make_v4f(5.0f, 5.0f, 5.0f, 5.0f) },
		.spacing           = { true, 5.0f },
		.radius            = { true, 10.0f },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.normal, "knob", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, make_v4f(5.0f, 5.0f, 5.0f, 5.0f) },
		.spacing           = { true, 5.0f },
		.radius            = { true, 25.0f },
		.align             = { true, ui_align_centre },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.hovered, "knob", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x252839, 255) },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.active, "knob", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, "picture", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, make_v4f(3.0f, 3.0f, 3.0f, 3.0f) },
		.align             = { true, ui_align_left },
		.max_size          = { true, { 100.0f, 100.0f } },
		.min_size          = { true, { 100.0f, 100.0f } },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.hovered, "picture", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xd9d9ff, 255) },
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, "picture", ((struct ui_style) {
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, "tree_button", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, make_v4f(3.0f, 3.0f, 3.0f, 3.0f) },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.hovered, "tree_button", ((struct ui_style) {
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, "tree_button", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, "tree_header", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, make_v4f(3.0f, 3.0f, 3.0f, 3.0f) },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.hovered, "tree_header", ((struct ui_style) {
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, "tree_header", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, "tree_container", ((struct ui_style) {
		.padding           = { true, make_v4f(10.0f, 0.0f, 0.0f, 0.0f) },
		.spacing           = { true, 5.0f },
		.radius            = { true, 0.0f }
	}));

	table_set(default_stylesheet.normal, "picker", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x262626, 255) },
		.max_size          = { true, make_v2f(150.0f, 150.0f) },
		.min_size          = { true, make_v2f(150.0f, 150.0f) },
		.padding           = { true, 3.0f },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.normal, "combo", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, make_v4f(3.0f, 3.0f, 3.0f, 3.0f) },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	table_set(default_stylesheet.hovered, "combo", ((struct ui_style) {
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, "combo", ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, "combo_container", ((struct ui_style) {
		.padding           = { true, make_v4f(5.0f, 10.0f, 5.0f, 5.0f) },
		.background_colour = { true, make_rgba(0x0c0c0c, 255) },
		.spacing           = { true, 5.0f },
		.radius            = { true, 0.0f },
		.outline_thickness = { true, 1.0f },
		.outline_colour    = { true, make_rgba(0x000000, 255) }
	}));

	reg_res_type("stylesheet", &(struct res_config) {
		.payload_size = sizeof(struct ui_stylesheet),
		.free_raw_on_load = true,
		.terminate_raw = true,
		.alt_raw = "normal = { }",
		.alt_raw_size = 13,
		.on_load = stylesheet_on_load,
		.on_unload = stylesheet_on_unload
	});

	default_font = new_font(bir_DejaVuSans_ttf, bir_DejaVuSans_ttf_size, 14.0f);
}

void ui_deinit() {
	free_table(default_stylesheet.normal);
	free_table(default_stylesheet.hovered);
	free_table(default_stylesheet.active);

	free_font(default_font);
}

struct ui* new_ui(const struct framebuffer* framebuffer) {
	struct ui* ui = core_calloc(1, sizeof(struct ui));

	ui->renderer = new_ui_renderer(framebuffer);
	ui->cmd_buffer = core_alloc(1024);

	ui->stylesheet = &default_stylesheet;
	ui->font = default_font;

	struct image alpha_image;
	init_image_from_raw(&alpha_image, bir_checkerboard_png, bir_checkerboard_png_size);

	ui->alpha_texture = video.new_texture(&alpha_image, texture_flags_none, texture_format_rgba8i);

	deinit_image(&alpha_image);

	return ui;
}

void free_ui(struct ui* ui) {
	for (u64* i = table_first(ui->container_meta); i; i = table_next(ui->container_meta, *i)) {
		struct ui_container_meta* m = table_get(ui->container_meta, *i);

		free_vector(m->cmd_views);
	}

	core_free(ui->wrap_buffer);
	core_free(ui->temp_str);
	free_vector(ui->columns);
	free_vector(ui->container_stack);
	free_vector(ui->cmd_free_queue);
	free_vector(ui->sorted_containers);
	free_table(ui->open_treenodes);
	free_table(ui->number_input_trailing_fullstops);
	free_table(ui->container_meta);
	free_ui_renderer(ui->renderer);
	video.free_texture(ui->alpha_texture);
	core_free(ui->cmd_buffer);
	core_free(ui);
}

struct ui_renderer* get_ui_renderer(const struct ui* ui) {
	return ui->renderer;
}

struct ui_stylesheet* load_stylesheet(const char* filename, struct resource* r) {
	struct resource temp;

	if (!r) {
		r = &temp;
	}

	*r = res_load("stylesheet", filename, null);
	return r->payload;
}

void ui_begin(struct ui* ui) {
	ui->cmd_buffer_idx = 0;

	ui->current_z = 0;

	ui->hovered = 0;

	ui_font(ui, null);

	v2i window_size = get_window_size();
	ui_renderer_clip(ui->renderer, make_v4i(0, 0, window_size.x, window_size.y));

	ui->cursor_pos = make_v2f(0.0f, 0.0f);
	ui->current_item_height = 0.0f;

	ui->treenode_id = 0;
	ui->container_id = 0;

	u64 to_delete[256];
	usize to_delete_count = 0;
	for (u64* i = table_first(ui->container_meta); i; i = table_next(ui->container_meta, *i)) {
		struct ui_container_meta* m = table_get(ui->container_meta, *i);
		m->visible = false;
		m->life--;
		if (m->life <= 0) {
			to_delete[to_delete_count++] = *i;
		}
	}

	for (usize i = 0; i < to_delete_count; i++) {
		struct ui_container_meta* m = table_get(ui->container_meta, to_delete[i]);
		free_vector(m->cmd_views);
		table_delete(ui->container_meta, to_delete[i]);
	}

	ui_begin_container(ui, make_v4f(0.0f, 0.0f, 1.0f, 1.0f), false);

	ui_columns(ui, 1, (f32[]) { 1.0f });
}

static i32 container_z_cmp(const void* a_v, const void* b_v) {
	const struct ui_container_meta* a = *(void**)a_v;
	const struct ui_container_meta* b = *(void**)b_v;

	return b->z - a->z;
}

static i32 rev_container_z_cmp(const void* a_v, const void* b_v) {
	const struct ui_container_meta* a = *(void**)a_v;
	const struct ui_container_meta* b = *(void**)b_v;

	return b->z - a->z;
}

void ui_end(struct ui* ui) {
	ui_end_container(ui);

	if (mouse_btn_just_released(mouse_btn_left)) {
		ui->dragging = 0;

		if (!ui->hovered) {
			ui->active = 0;
		}
	}

	vector_clear(ui->sorted_containers);

	for (u64* i = table_first(ui->container_meta); i; i = table_next(ui->container_meta, *i)) {
		struct ui_container_meta* m = table_get(ui->container_meta, *i);
		m->interactable = false;
		if (m->visible) {
			vector_push(ui->sorted_containers, m);
		}
	}

	/* Sort all of the containers. */
	qsort(ui->sorted_containers, vector_count(ui->sorted_containers), sizeof *ui->sorted_containers, container_z_cmp);

	/* Get all of the hovered containers. */
	struct ui_container_meta* hovered_containers[16];
	usize hovered_container_count = 0;
	for (usize i = 0; i < vector_count(ui->sorted_containers); i++) {
		struct ui_container_meta* meta = ui->sorted_containers[i];
		if (mouse_over_rect(meta->position, meta->dimensions)) {
			hovered_containers[hovered_container_count++] = meta;
		}
	}

	if (hovered_container_count > 0) {
		qsort(hovered_containers, hovered_container_count, sizeof *hovered_containers, rev_container_z_cmp);

		struct ui_container_meta* top_meta = hovered_containers[0];

		top_meta->interactable = true;
	}
}

static struct ui_container_meta* get_container_meta(struct ui* ui, u64 id) {
	struct ui_container_meta* meta = table_get(ui->container_meta, id);
	if (!meta) {
		table_set(ui->container_meta, id, (struct ui_container_meta) { 0 });
		meta = table_get(ui->container_meta, id);
		meta->z = 0;
		meta->life = 1024;
		meta->cmd_views = null;
	}
	return meta;
}

void ui_begin_container_ex(struct ui* ui, const char* class, v4f rect, bool scrollable) {
	struct ui_container* parent = null;
	if (vector_count(ui->container_stack) > 0) {
		parent = vector_end(ui->container_stack) - 1;
	}

	u64 id = ui->container_id++;
	struct ui_container_meta* meta = get_container_meta(ui, id);
	meta->life = 1024;
	vector_clear(meta->cmd_views);
	vector_push(meta->cmd_views, (struct ui_cmd_view) {
		.head = ui->cmd_buffer_idx
	});
	meta->current_view = vector_end(meta->cmd_views) - 1;

	ui->current_z++;
	meta->z = ui->current_z;
	meta->floating = false;
	meta->visible = true;

	v2i window_size = get_window_size();

	v2f scroll = make_v2f(0.0f, 0.0f);
	if (scrollable) {
		scroll = meta->scroll;
	} else {
		scroll = make_v2f(0.0f, 0.0f);
	}

	f32 pad_top_bottom = 0.0f;
	v4f padding = make_v4f(0.0f, 0.0f, 0.0f, 0.0f);
	v4f clip;
	f32 spacing = 5.0f;
	if (parent) {
		struct ui_container_meta* parent_meta = get_container_meta(ui, parent->id);
		parent_meta->current_view->tail = ui->cmd_buffer_idx;

		const struct ui_style style = ui_get_style(ui, "container", class, ui_style_variant_none);
		pad_top_bottom = parent->padding.y;
		padding = v4f_scale(style.padding.value, get_dpi_scale());
		spacing = style.spacing.value * get_dpi_scale();

		clip = make_v4f(
			rect.x * (f32)(parent->rect.z) + parent->left_bound,
			rect.y * (f32)(parent->rect.w) + style.padding.value.y + parent->rect.y,
			rect.z * (f32)(parent->rect.z) - (style.padding.value.x + style.padding.value.z),
			rect.w * (f32)(parent->rect.w) - (style.padding.value.y + style.padding.value.w));

		ui_clip(ui, clip);
		ui_draw_rect(ui, make_v2f(clip.x, clip.y), make_v2f(clip.z, clip.w), style.background_colour.value, style.radius.value);

		if (style.outline_thickness.value > 0.0f) {
			ui_draw_outline(ui,
				make_v2f(clip.x, clip.y), make_v2f(clip.z, clip.w),
				style.outline_colour.value, style.radius.value,
				style.outline_thickness.value);
		}
	} else {
		clip = make_v4f(
			rect.x * (f32)window_size.x + 5.0f,
			rect.y * (f32)window_size.y,
			rect.z * (f32)window_size.x,
			rect.w * (f32)window_size.y);
		ui_clip(ui, clip);
	}

	meta->position = make_v2f(clip.x, clip.y);
	meta->dimensions = make_v2f(clip.z, clip.w);

	vector_push(ui->container_stack, ((struct ui_container) {
		.rect = clip,
		.padding = padding,
		.spacing = spacing,
		.id = id,
		.scrollable = scrollable,
		.left_bound = clip.x + padding.x + scroll.x,
		.content_size = make_v2f(0.0f, 0.0f),
		.interactable = meta->interactable,
		.z = meta->z
	}));

	ui->cursor_pos = make_v2f(clip.x + padding.x + scroll.x, clip.y + padding.y + scroll.y);
}

void ui_begin_floating_container_ex(struct ui* ui, const char* class, v4f rect, bool scrollable) {
	u64 id = ui->container_id++;

	struct ui_container_meta* meta = get_container_meta(ui, id);

	struct ui_container* parent = null;
	if (vector_count(ui->container_stack) > 0) {
		parent = vector_end(ui->container_stack) - 1;
	}

	v2f scroll = make_v2f(0.0f, 0.0f);
	if (scrollable) {
		scroll = meta->scroll;
	} else {
		scroll = make_v2f(0.0f, 0.0f);
	}

	const struct ui_style style = ui_get_style(ui, "container", class, ui_style_variant_none);

	ui->current_z++;

	meta->position = make_v2f(rect.x, rect.y);
	meta->dimensions = make_v2f(rect.z, rect.w);
	meta->floating = true;
	meta->visible = true;
	meta->z = ui->current_z;
	meta->life = 1024;
	vector_clear(meta->cmd_views);
	vector_push(meta->cmd_views, (struct ui_cmd_view) {
		.head = ui->cmd_buffer_idx
	});
	meta->current_view = vector_end(meta->cmd_views) - 1;

	ui_clip(ui, rect);
	ui_draw_rect(ui,
		make_v2f(rect.x, rect.y), make_v2f(rect.z, rect.w),
		style.background_colour.value, style.radius.value);

	if (parent) {
		struct ui_container_meta* parent_meta = get_container_meta(ui, parent->id);
		parent_meta->current_view->tail = ui->cmd_buffer_idx;
	}

	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			make_v2f(rect.x, rect.y), make_v2f(rect.z, rect.w),
			style.outline_colour.value, style.radius.value,
			style.outline_thickness.value);
	}

	vector_push(ui->container_stack, ((struct ui_container) {
		.rect = rect,
		.padding = v4f_scale(style.padding.value, get_dpi_scale()),
		.spacing = style.spacing.value * get_dpi_scale(),
		.id = id,
		.scrollable = scrollable,
		.left_bound = rect.x + style.padding.value.x * get_dpi_scale() + scroll.x,
		.content_size = make_v2f(0.0f, 0.0f),
		.old_cursor_pos = ui->cursor_pos,
		.interactable = meta->interactable,
		.z = meta->z
	}));

	ui->cursor_pos = make_v2f(rect.x + style.padding.value.x + scroll.x, rect.y + style.padding.value.y + scroll.y);
}

void ui_end_container(struct ui* ui) {
	struct ui_container* container = vector_pop(ui->container_stack);

	struct ui_container_meta* meta = get_container_meta(ui, container->id);
	meta->current_view->tail = ui->cmd_buffer_idx;

	container->content_size.x += container->padding.x;

	if (container->scrollable && container->interactable && mouse_over_rect(make_v2f(container->rect.x, container->rect.y), make_v2f(container->rect.z, container->rect.w))) {
		meta->scroll.x += (f32)get_scroll().x * 10.0f;
		meta->scroll.y += (f32)get_scroll().y * get_font_height(ui->font) * 3.0f;

		v2f max_scroll = get_container_max_scroll(container, meta);

		if (meta->scroll.y < -max_scroll.y) {
			meta->scroll.y = -max_scroll.y;
		}

		if (meta->scroll.y > 0.0f) {
			meta->scroll.y = 0.0f;
		}

		if (meta->scroll.x < -max_scroll.x) {
			meta->scroll.x = -max_scroll.x;
		}

		if (meta->scroll.x > 0.0f) {
			meta->scroll.x = 0.0f;
		}
	}
	
	if (container->floating) {
		ui->cursor_pos = container->old_cursor_pos;
	} else {
		ui->cursor_pos.x -= container->padding.x;
		ui->cursor_pos.y += container->padding.y;
	}

	if (vector_count(ui->container_stack) > 0) {
		struct ui_container* parent = vector_end(ui->container_stack) - 1;

		struct ui_container_meta* parent_meta = get_container_meta(ui, parent->id);
		vector_push(parent_meta->cmd_views, (struct ui_cmd_view) {
			.head = ui->cmd_buffer_idx
		});
		parent_meta->current_view = vector_end(parent_meta->cmd_views) - 1;

		ui_clip(ui, parent->rect);
	}

	ui->current_z--;
}

void ui_font(struct ui* ui, struct font* font) {
	ui->font = font ? font : default_font;
}

void ui_stylesheet(struct ui* ui, struct ui_stylesheet* ss) {
	ui->stylesheet = ss ? ss : &default_stylesheet;
}

static v2f get_ui_el_position(struct ui* ui, const struct ui_style* style, v2f dimensions) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	f32 x_margin = 0.0f;
	for (usize i = 0; i < ui->column + 1; i++) {
		x_margin += ui->columns[i] * container->rect.z;
	}

	switch (style->align.value) {
		case ui_align_left:
			return ui->cursor_pos;
		case ui_align_right:
			return make_v2f(((container->left_bound - container->padding.x) + x_margin) - dimensions.x - container->padding.z, ui->cursor_pos.y);
		case ui_align_centre: {
			f32 x_inner_margin = x_margin - ui->columns[ui->column] * container->rect.z;
			return make_v2f(((container->left_bound - container->padding.x) + x_inner_margin * 0.5f + x_margin * 0.5f) - (dimensions.x * 0.5f), ui->cursor_pos.y);
		}
		default:
			return make_v2f(0.0f, 0.0f);
	}
}

void ui_advance(struct ui* ui, v2f dimensions) {
	struct ui_container* container = vector_end(ui->container_stack) - 1;

	ui->current_item_height = cr_max(ui->current_item_height, dimensions.y);
	v2f last = ui->cursor_pos;
	if (dimensions.y > ui->current_item_height) {
		ui->current_item_height = dimensions.y;
	}

	container->content_size.x = cr_max(container->content_size.x, (ui->cursor_pos.x - container->rect.x) + dimensions.x);

	ui->cursor_pos.x += ui->columns[ui->column] * container->rect.z;
	ui->column++;

	if (ui->column >= ui->column_count) {
		ui->cursor_pos.x = container->left_bound;
		ui->cursor_pos.y += ui->current_item_height;
		container->content_size.y += ui->cursor_pos.y - last.y;
		container->content_size.x = cr_max(container->content_size.x, (ui->cursor_pos.x - container->rect.x) + dimensions.x);
		ui->column = 0;
		ui->current_item_height = 0.0f;
	}
}

v4f ui_get_container_rect(struct ui* ui) {
	struct ui_container* container = vector_end(ui->container_stack) - 1;
	return container->rect;
}

void ui_scroll_max(struct ui* ui) {
	struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_container_meta* meta = get_container_meta(ui, container->id);

	if (container->scrollable && meta) {
		v2f max_scroll = get_container_max_scroll(container, meta);

		meta->scroll.y = max_scroll.y;
	}
}

void ui_scroll_min(struct ui* ui) {
	struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_container_meta* meta = get_container_meta(ui, container->id);

	if (container->scrollable && meta) {
		meta->scroll.y = 0.0f;
	}
}

void ui_columns(struct ui* ui, usize count, f32* columns) {
	vector_clear(ui->columns);

	for (usize i = 0; i < count; i++) {
		vector_push(ui->columns, columns[i]);
	}

	ui->column_count = count;
	ui->column = 0;
	ui->current_item_height = 0.0f;
}

static i32 word_wrap_len(struct font* font, const char* string, i32 width) {
	i32 i = 0;

	i32 string_len = (i32)strlen(string);
	i32 line_start = 0;

	i32 len = 0;

	while (i < string_len) {
		for (i32 c = 1; c < width - 8; c += get_char_dimensions(font, string + i).x) {
			if (i >= string_len) {
				len++;
				return len;
			}

			len++;

			if (string[i] == '\n') {
				line_start = i;
				c = 1;
			}

			i++;
		}

		if (isspace(string[i])) {
			i++;
			len++;
			line_start = i;
		} else {
			for (i32 c = i; c > 0; c--) {
				if (isspace(string[c])) {
					i = c + 1;
					line_start = i;
					break;
				}

				if (c <= line_start) {
					i++;
					len++;
					break;
				}
			}
		}
	}

	len++;

	return len;
}

static char* word_wrap(struct font* font, char* buffer, const char* string, i32 width) {
	i32 i = 0;

	i32 string_len = (i32)strlen(string);
	i32 line_start = 0;

	while (i < string_len) {
		for (i32 c = 1; c < width - 8; c += get_char_dimensions(font, string + i).x) {
			if (i >= string_len) {
				buffer[i] = '\0';
				return buffer;
			}

			buffer[i] = string[i];

			if (string[i] == '\n') {
				line_start = i;
				c = 1;
			}

			i++;
		}

		if (isspace(string[i])) {
			buffer[i++] = '\n';
			line_start = i;
		} else {
			for (i32 c = i; c > 0; c--) {
				if (isspace(string[c])) {
					buffer[c] = '\n';
					i = c + 1;
					line_start = i;
					break;
				}

				if (c <= line_start) {
					buffer[i++] = '\n';
					break;
				}
			}
		}
	}

	buffer[i] = '\0';

	return buffer;
}

bool ui_text_ex(struct ui* ui, const char* class, const char* text, bool wrapped) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_style style = ui_get_style(ui, "label", class, ui_style_variant_none);

	/* TODO: Do this more cleanly. That is, take into account Unicode and don't
	 * malloc every frame. I did this quickly on a short time budget. */
	if (wrapped) {
		const v2f wrap_dim = v2f_add(make_v2f(ui->columns[ui->column] *
				container->rect.z - (style.padding.value.x * 2.0f + style.padding.value.z) -
				(container->padding.x + container->padding.z), get_font_height(ui->font)),
			make_v2f(0.0f, style.padding.value.y + style.padding.value.w));

		i32 wrap_count = word_wrap_len(ui->font, text, wrap_dim.x);
		if (wrap_count > (i32)ui->wrap_buffer_size) {
			ui->wrap_buffer = core_realloc(ui->wrap_buffer, wrap_count + 1);
		}

		strcpy(ui->wrap_buffer, text);

		word_wrap(ui->font, ui->wrap_buffer, text, (i32)wrap_dim.x);

		text = ui->wrap_buffer;
	}

	const v2f text_dimensions = get_text_dimensions(ui->font, text);
	const v2f dimensions = make_v2f(text_dimensions.x + style.padding.value.x + style.padding.value.z,
		text_dimensions.y + style.padding.value.y + style.padding.value.w);

	ui_draw_rect(ui, get_ui_el_position(ui, &style, dimensions), make_v2f(dimensions.x, dimensions.y),
		style.background_colour.value, style.radius.value);
	struct ui_cmd_draw_rect* rect_cmd = ui_last_cmd(ui);

	ui_draw_text(ui, v2f_add(rect_cmd->position, style.padding.value),
		text_dimensions, text, style.text_colour.value);
	struct ui_cmd_draw_text* text_cmd = ui_last_cmd(ui);

	bool hovered = container->interactable && mouse_over_rect(rect_cmd->position, dimensions);
	if (ui_get_style_variant(ui, &style, "label", class, hovered, false)) {
		ui->hovered = hash_string(text);

		rect_cmd->position = get_ui_el_position(ui, &style, dimensions);
		rect_cmd->radius   = style.radius.value;
		text_cmd->position = v2f_add(rect_cmd->position, style.padding.value);

		rect_cmd->colour = style.background_colour.value;
		text_cmd->colour = style.text_colour.value;
	}

	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			rect_cmd->position,
			rect_cmd->dimensions,
			style.outline_colour.value, style.radius.value,
			style.outline_thickness.value);
	}

	ui_advance(ui,
		make_v2f(dimensions.x + style.outline_thickness.value * 2.0f,
		dimensions.y + container->spacing + style.outline_thickness.value * 2.0f));

	if (hovered && mouse_btn_just_released(mouse_btn_left)) { return true; }
	return false;
}

void ui_linebreak(struct ui* ui) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	ui->column = 0;
	ui->cursor_pos.y += get_font_height(ui->font) + container->spacing;
	ui->cursor_pos.x = container->left_bound;
}

bool ui_knob_ex(struct ui* ui, const char* class, f32* val, f32 min, f32 max) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	const u64 id = elf_hash((const u8*)&val, sizeof val);

	struct ui_style style = ui_get_style(ui, "knob", class, ui_style_variant_none);
	style.radius.value *= get_dpi_scale();

	const f32 handle_radius = 5.0f * get_dpi_scale();

	const v2f dimensions = make_v2f(style.radius.value * 2.0f, style.radius.value * 2.0f);
	const v2f position = get_ui_el_position(ui, &style, dimensions);

	ui_draw_circle(ui, position, style.radius.value, style.background_colour.value);
	struct ui_cmd_draw_circle* knob_cmd = ui_last_cmd(ui);
	ui_draw_circle(ui, make_v2f(0.0f, 0.0f), handle_radius, style.text_colour.value);
	struct ui_cmd_draw_circle* handle_cmd = ui_last_cmd(ui);

	bool hovered = container->interactable && mouse_over_circle(position, knob_cmd->radius);

	v2f knob_origin = make_v2f(
		knob_cmd->position.x + style.radius.value - handle_radius,
		knob_cmd->position.y + style.radius.value - handle_radius);

	f32 min_angle = to_rad(40.0f);
	f32 max_angle = to_rad(320.0f);
	f32 angle = lerp(min_angle, max_angle, map(*val, max, min, 0.0f, 1.0f));

	if (hovered) {
		ui->hovered = id;

		if (mouse_btn_just_pressed(mouse_btn_left)) {
			ui->dragging = id;

			v2i mouse_pos = get_mouse_pos();
			v2f v = v2f_sub(make_v2f(mouse_pos.x, mouse_pos.y), knob_origin);
			f32 to_mouse_angle = atan2f(-v.x, -v.y) + to_rad(180.0f);

			to_mouse_angle = clamp(to_mouse_angle, min_angle, max_angle);

			ui->knob_angle_offset = angle - to_mouse_angle;
		}
	}

	bool changed = false;

	if (ui->dragging == id) {
		style = ui_get_style(ui, "knob", class, ui_style_variant_active);
		style.radius.value *= get_dpi_scale();

		knob_cmd->radius   = style.radius.value;
		knob_cmd->position = get_ui_el_position(ui, &style, dimensions);
		knob_cmd->colour   = style.background_colour.value;

		handle_cmd->colour = style.text_colour.value;

		knob_origin = make_v2f(
			knob_cmd->position.x + style.radius.value - handle_radius,
			knob_cmd->position.y + style.radius.value - handle_radius);

		v2i mouse_pos = get_mouse_pos();
		v2f v = v2f_sub(make_v2f(mouse_pos.x, mouse_pos.y), knob_origin);
		f32 old_angle = angle;
		angle = atan2f(-v.x, -v.y) + to_rad(180.0f);

		angle += ui->knob_angle_offset;
		angle = clamp(angle, min_angle, max_angle);

		/* If the difference between the old angle and the new angle is too great,
		 * then assume that the knob has been rotated from 0 to the maximum, which
		 * shouldn't be allowed, so reset the angle to the angle before any movement.
		 *
		 * Theoretically this could also be caused by the user moving his or her mouse
		 * very quickly at a low framerate, but in practice this is a much more minor
		 * issue than we are trying to solve, so it can be effectively ignored. */
		if (fabsf(old_angle - angle) > 2.0f) {
			angle = old_angle;
		}

		*val = map(angle, min_angle, max_angle, max, min);
		*val = clamp(*val, min, max);

		changed = true;
	} else if (ui_get_style_variant(ui, &style, "knob", class, hovered, false)) {
		style.radius.value *= get_dpi_scale();

		knob_cmd->radius   = style.radius.value;
		knob_cmd->position = get_ui_el_position(ui, &style, dimensions);
		knob_cmd->colour   = style.background_colour.value;

		handle_cmd->colour = style.text_colour.value;
	}

	/* Rotate the handle based on `val' around the centre of the knob.
	 * The background circle doesn't need rotating, as it doesn't have
	 * a background and any rotation will be invisible. */
	f32 angle_rad = angle;
	v2f rotation = make_v2f(
		sinf(angle),
		cosf(angle)
	);

	handle_cmd->position = v2f_add(
		knob_origin,
		v2f_mul(
			rotation,
			make_v2f(style.radius.value - handle_radius - style.padding.value.x,
			style.radius.value - handle_radius - style.padding.value.y)));
	
	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui, knob_cmd->position, v2f_scale(make_v2f(knob_cmd->radius, knob_cmd->radius), 2.0f),
			style.outline_colour.value, knob_cmd->radius,
			style.outline_thickness.value);

		ui_draw_outline(ui, handle_cmd->position, v2f_scale(make_v2f(handle_cmd->radius, handle_cmd->radius), 2.0f),
			style.outline_colour.value, handle_cmd->radius,
			style.outline_thickness.value);
	}

	ui_advance(ui, make_v2f(dimensions.x, dimensions.y + container->spacing));

	return changed;
}

bool ui_picture_ex(struct ui* ui, const char* class, const struct texture* texture, v4i rect) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_style style = ui_get_style(ui, "picture", class, ui_style_variant_none);

	v2i texture_size = video.get_texture_size(texture);
	v2f dimensions = make_v2f(texture_size.x, texture_size.y);

	if (style.max_size.has_value) {
		dimensions.x = cr_min(dimensions.x, style.max_size.value.x);
		dimensions.y = cr_min(dimensions.y, style.max_size.value.y);
	}

	if (style.min_size.has_value) {
		dimensions.x = cr_max(dimensions.x, style.min_size.value.x);
		dimensions.y = cr_max(dimensions.y, style.min_size.value.y);
	}

	const v2f rect_dimensions = make_v2f(dimensions.x + style.padding.value.x + style.padding.value.z,
		dimensions.y + style.padding.value.y + style.padding.value.w);

	ui_draw_rect(ui, get_ui_el_position(ui, &style, dimensions),
		rect_dimensions,
		style.background_colour.value, style.radius.value);
	struct ui_cmd_draw_rect* rect_cmd = ui_last_cmd(ui);

	ui_draw_texture(ui, v2f_add(rect_cmd->position, style.padding.value), dimensions,
		texture, rect, style.text_colour.value, style.radius.value);
	struct ui_cmd_texture* text_cmd = ui_last_cmd(ui);

	bool hovered = container->interactable && mouse_over_rect(rect_cmd->position, rect_dimensions);
	if (ui_get_style_variant(ui, &style, "picture", class, hovered, false)) {
		rect_cmd->position = get_ui_el_position(ui, &style, dimensions);
		rect_cmd->radius   = style.radius.value;
		text_cmd->position = v2f_add(rect_cmd->position, style.padding.value);
		text_cmd->radius   = style.radius.value;

		rect_cmd->colour = style.background_colour.value;
		text_cmd->colour = style.text_colour.value;
	}

	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			rect_cmd->position,
			rect_cmd->dimensions,
			style.outline_colour.value, style.radius.value,
			style.outline_thickness.value);
	}

	ui_advance(ui, make_v2f(rect_cmd->dimensions.x, rect_cmd->dimensions.y + container->spacing));

	if (hovered && mouse_btn_just_released(mouse_btn_left)) { return true; }
	return false;
}

bool ui_text_input_filter(char c) {
	return true;
}

bool ui_number_input_filter(char c) {
	return (c >= '0' && c <= '9') || c == '.';
}

bool ui_alphanum_input_filter(char c) {
	return
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		ui_number_input_filter(c);
}

bool ui_input_ex2(struct ui* ui, const char* class, char* buf, usize buf_size, ui_input_filter filter, u64 id, bool is_password) {
	if (id == 0) {
		id = elf_hash((const u8*)&buf, sizeof buf);
	}

	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_style style = ui_get_style(ui, "input", class, ui_style_variant_none);

	const v2f dimensions = v2f_add(make_v2f(ui->columns[ui->column] *
		container->rect.z - (style.padding.value.x * 2.0f + style.padding.value.z) - (container->padding.x + container->padding.z), get_font_height(ui->font)),
		make_v2f(0.0f, style.padding.value.y + style.padding.value.w));

	usize buf_len = strlen(buf);

	v2f star_size;
	v2f text_dimensions;
	
	ui_draw_rect(ui, get_ui_el_position(ui, &style, dimensions), dimensions,
		style.background_colour.value, style.radius.value);
	struct ui_cmd_draw_rect* rect_cmd = ui_last_cmd(ui);

	struct ui_cmd_draw_rect* cursor_cmd = null;

	bool hovered = container->interactable && mouse_over_rect(rect_cmd->position, dimensions);

	if (hovered) {
		ui->hovered = id;

		if (mouse_btn_just_released(mouse_btn_left)) {
			ui->active = id;
			ui->input_cursor = (u32)strlen(buf);
			ui->input_select_start = ui->input_cursor;
		}
	}

	bool active = ui->active == id;

	if (ui_get_style_variant(ui, &style, "input", class, hovered, active)) {
		rect_cmd->position = get_ui_el_position(ui, &style, dimensions);
		rect_cmd->radius   = style.radius.value;

		rect_cmd->colour = style.background_colour.value;
	}

	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			rect_cmd->position,
			rect_cmd->dimensions,
			style.outline_colour.value, style.radius.value,
			style.outline_thickness.value);
	}

	const v2f text_pos = v2f_add(rect_cmd->position, style.padding.value);

	bool submitted = false;

	v4f prev_clip = ui->current_clip;
	ui_clip(ui, make_v4f(text_pos.x, text_pos.y, dimensions.x - style.padding.value.x, dimensions.y - style.padding.value.y));

	f32 scroll = 0.0f;
	if (active) {
		usize input_len;
		const char* input_string;
		bool reset_select = false;

		bool input = get_input_string(&input_string, &input_len);

		if (ui->input_cursor != ui->input_select_start) {
			i32 start = ui->input_select_start;
			i32 end = ui->input_cursor;

			if (start > end) {
				i32 temp = start;
				start = end;
				end = temp;
			}

			i32 range = end - start;
			
			if ((key_just_pressed(key_C) || key_just_pressed(key_X)) && key_pressed(key_control)) {
				set_clipboard_text(&buf[start], (usize)(end - start));
			}

			if (input || key_just_pressed(key_backspace) || key_just_pressed(key_delete) ||
				((key_just_pressed(key_V) || key_just_pressed(key_X)) && key_pressed(key_control))) {
				for (i32 i = start; i < ((i32)buf_len - range) + 1; i++) {
					buf[i] = buf[i + range];
				}

				ui->input_cursor = start;
				ui->input_select_start = ui->input_cursor;
			}
		} else if (ui->input_cursor > 0 && key_just_pressed(key_backspace)) {
			for (u32 i = ui->input_cursor - 1; i < buf_len - 1; i++) {
				buf[i] = buf[i + 1];
			}

			buf[buf_len - 1] = '\0';
			ui->input_cursor--;

			reset_select = true;
		} else if (ui->input_cursor < buf_len && buf_len > 0 && key_just_pressed(key_delete)) {
			for (u32 i = ui->input_cursor; i < buf_len - 1; i++) {
				buf[i] = buf[i + 1];
			}

			buf[buf_len - 1] = '\0';

			reset_select = true;
		}

		u32 old_input_cursor = ui->input_cursor;

		bool selecting = false;

		if (input && buf_len + input_len < buf_size) {
			for (i64 i = buf_len; i >= ui->input_cursor; i--) {
				buf[i + input_len] = buf[i];
			}

			for (usize i = 0; i < input_len; i++) {
				if (i + ui->input_cursor >= buf_size) { break; }

				if (filter(input_string[i])) {
					buf[i + ui->input_cursor] = input_string[i];
				}
			}

			ui->input_cursor += (u32)input_len;
		} else {
			selecting = key_pressed(key_shift);
		}

		if (key_just_pressed(key_V) && key_pressed(key_control)) {
			char b[256];
			get_clipboard_text(b, sizeof b);

			usize copy_len = strlen(b);

			for (i64 i = buf_len; i >= ui->input_cursor; i--) {
				buf[i + copy_len] = buf[i];
			}

			for (usize i = 0; i < copy_len; i++) {
				if (i + ui->input_cursor >= buf_size) { break; }

				if (filter(b[i])) {
					buf[i + ui->input_cursor] = b[i];
				}
			}

			ui->input_cursor += (u32)copy_len;
		}

		if (key_just_pressed(key_return)) {
			submitted = true;
			ui->active = 0;
			active = false;
		}

		if (ui->input_cursor > 0 && key_just_pressed(key_left)) {
			if (key_pressed(key_control)) {
				i32 i;
				ui->input_cursor--;
				for (i = (i32)ui->input_cursor; i > 0 && buf[i] != ' '; i--, ui->input_cursor--) {}
			} else {
				ui->input_cursor--;
			}
		}

		if (ui->input_cursor < buf_len && key_just_pressed(key_right)) {
			if (key_pressed(key_control)) {
				ui->input_cursor++;
				for (i32 i = ui->input_cursor; i < buf_len && buf[i] != ' '; i++, ui->input_cursor++) {}
			} else {
				ui->input_cursor++;
			}
		}

		if (key_just_pressed(key_home)) {
			ui->input_cursor = 0;
		} else if (key_just_pressed(key_end)) {
			ui->input_cursor = (u32)buf_len;
		}

		if (old_input_cursor != ui->input_cursor && !selecting || reset_select) {
			ui->input_select_start = ui->input_cursor;
		}

		if (key_just_pressed(key_A) && key_pressed(key_control)) {
			ui->input_select_start = 0;
			ui->input_cursor = (u32)buf_len;
		}

		f32 cursor_x_off;
		f32 select_x_off;

		if (is_password) {
			star_size = get_char_dimensions(ui->font, "*");
			text_dimensions = make_v2f(star_size.x * buf_len, get_font_height(ui->font));
			cursor_x_off = star_size.x * (f32)ui->input_cursor;
			select_x_off = star_size.x * (f32)ui->input_select_start;
		} else {
			text_dimensions = get_text_dimensions(ui->font, buf);
			cursor_x_off = get_text_n_dimensions(ui->font, buf, ui->input_cursor).x;
			select_x_off = get_text_n_dimensions(ui->font, buf, ui->input_select_start).x;
		}

		scroll = cr_min(dimensions.x - cursor_x_off - style.padding.value.x - get_font_height(ui->font), 0.0f);

		ui_draw_rect(ui, v2f_add(text_pos, make_v2f(cursor_x_off + scroll, 0.0f)), make_v2f(1.0f * get_dpi_scale(), get_font_height(ui->font)),
			style.text_colour.value, 0.0f);
		cursor_cmd = ui_last_cmd(ui);

		if (ui->input_select_start != ui->input_cursor) {
			f32 x_start = cursor_x_off + scroll;
			f32 x_end = select_x_off + scroll;

			if (x_start > x_end) {
				f32 t = x_start;
				x_start = x_end;
				x_end = t;
			}

			ui_draw_rect(ui, v2f_add(text_pos, make_v2f(x_start, 0.0f)), make_v2f(x_end - x_start, get_font_height(ui->font)),
				style.select_colour.value, 0.0f);
		}
	} else {
		if (is_password) {
			star_size = get_char_dimensions(ui->font, "*");
			text_dimensions = make_v2f(star_size.x * buf_len, get_font_height(ui->font));
		} else {
			text_dimensions = get_text_dimensions(ui->font, buf);
		}
	}


	if (is_password) {	
		f32 x = text_pos.x + scroll;
		for (usize i = 0; i < buf_len; i++) {
			ui_draw_text(ui, make_v2f(x, text_pos.y), text_dimensions, "*", style.text_colour.value);
			x += star_size.x;
		}
	} else {
		ui_draw_text(ui, make_v2f(text_pos.x + scroll, text_pos.y), text_dimensions, buf, style.text_colour.value);
	}

	struct ui_cmd_draw_text* text_cmd = ui_last_cmd(ui);

	ui_clip(ui, prev_clip);

	ui_advance(ui, make_v2f(dimensions.x, dimensions.y + container->spacing));

	ui->last_input_id = id;
	ui->last_input_buf = buf;

	return submitted;
}

bool ui_number_input_ex(struct ui* ui, const char* class, f64* target) {
	char buf[64];

	u64 id = elf_hash((const u8*)&target, sizeof target);

	bool* trailing_ptr = table_get(ui->number_input_trailing_fullstops, id);

	sprintf(buf, "%g", *target);
	if (trailing_ptr && *trailing_ptr) {
		strcat(buf, ".");
	}

	bool submitted = ui_input_ex2(ui, class, buf, sizeof buf, ui_number_input_filter, id, false);
	if (buf[strlen(buf) - 1] == '.') {
		table_set(ui->number_input_trailing_fullstops, id, true); 
	} else {
		table_set(ui->number_input_trailing_fullstops, id, false);
	}

	*target = strtod(buf, null);

	return submitted;
}

bool ui_selectable_tree_node_ex(struct ui* ui, const char* class, const char* text, bool leaf, bool* selected, u64 id) {
	if (id == 0) {
		id = ui->treenode_id++ + elf_hash((const u8*)&selected, sizeof selected);
	}

	bool* open_ptr = table_get(ui->open_treenodes, id);
	if (!open_ptr) {
		table_set(ui->open_treenodes, id, false);
		open_ptr = table_get(ui->open_treenodes, id);
	}

	bool open = *open_ptr;

	struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_style button_style = ui_get_style(ui, "tree_button", class, ui_style_variant_none);

	const v2f text_dimensions = get_text_dimensions(ui->font, text);

	v2f header_pos;
	v2f button_pos = make_v2f(0.0f, 0.0f);
	v2f button_dimensions = make_v2f(0.0f, 0.0f);
	bool button_hovered = false;

	if (!leaf) {
		v2f outline_offset = make_v2f(button_style.outline_thickness.value, button_style.outline_thickness.value);

		button_dimensions = v2f_add(get_text_dimensions(ui->font, "+"), make_v2f(
			button_style.padding.value.x + button_style.padding.value.z,
			button_style.padding.value.y + button_style.padding.value.w));

		ui_draw_rect(ui, v2f_add(ui->cursor_pos, outline_offset), button_dimensions,
			button_style.background_colour.value, button_style.radius.value);
		struct ui_cmd_draw_rect* rect_cmd = ui_last_cmd(ui);

		button_hovered = container->interactable && mouse_over_rect(rect_cmd->position, button_dimensions);
		if (ui_get_style_variant(ui, &button_style, "tree_button", class, button_hovered, false)) {
			rect_cmd->radius    = button_style.radius.value;

			rect_cmd->colour = button_style.background_colour.value;
		}

		if (button_style.outline_thickness.value > 0.0f) {
			ui_draw_outline(ui,
				rect_cmd->position,
				rect_cmd->dimensions,
				button_style.outline_colour.value, button_style.radius.value,
				button_style.outline_thickness.value);
		}

		header_pos = make_v2f(rect_cmd->position.x + button_dimensions.x, rect_cmd->position.y);
		button_pos = rect_cmd->position;
	} else {
		header_pos = ui->cursor_pos;
	}

	struct ui_style background_style = ui_get_style(ui, "tree_header", class, ui_style_variant_none);

	const v2f background_dimensions = make_v2f(
		container->rect.z - (header_pos.x - container->rect.x) - container->padding.z,
		text_dimensions.y + background_style.padding.value.y + background_style.padding.value.w);

	ui_draw_rect(ui, header_pos,
		background_dimensions, background_style.background_colour.value, background_style.radius.value);
	struct ui_cmd_draw_rect* back_cmd = ui_last_cmd(ui);

	ui_draw_text(ui, v2f_add(header_pos, background_style.padding.value),
		text_dimensions, text, background_style.text_colour.value);
	struct ui_cmd_draw_text* text_cmd = ui_last_cmd(ui);
	bool back_hovered = container->interactable && mouse_over_rect(back_cmd->position, back_cmd->dimensions);

	if (selected && back_hovered && mouse_btn_just_released(mouse_btn_left)) {
		*selected = !*selected;
	}

	bool active = selected ? *selected : false;
	if (ui_get_style_variant(ui, &background_style, "tree_header", class, back_hovered, active)) {
		back_cmd->radius = background_style.radius.value;
		back_cmd->colour = background_style.background_colour.value;
		text_cmd->colour = background_style.text_colour.value;
	}

	if (button_style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			back_cmd->position,
			back_cmd->dimensions,
			background_style.outline_colour.value, background_style.radius.value,
			background_style.outline_thickness.value);
	}

	ui->cursor_pos.y += background_dimensions.y;
	container->content_size.y += background_dimensions.y;

	if (button_hovered && mouse_btn_just_released(mouse_btn_left)) {
		open = !open;
		table_set(ui->open_treenodes, id, open);
	}

	if (!leaf) {
		ui_draw_text(ui, v2f_add(button_pos, button_style.padding.value),
			text_dimensions, open ? " -" : "+", button_style.text_colour.value);
	}

	if (open && !leaf) {
		container->left_bound += 10.0f;
		ui->cursor_pos.x += 10.0f;

		ui_columns(ui, 1, (f32[]) { 1.0f });
	}

	ui->cursor_pos.y += container->spacing;
	container->content_size.y += container->spacing;

	return open;
}

void ui_tree_pop(struct ui* ui) {
	struct ui_container* container = vector_end(ui->container_stack) - 1;

	container->left_bound -= 10.0f;
	ui->cursor_pos.x -= 10.0f;
}

bool ui_combo_ex(struct ui* ui, const char* class, i32* item, const char** items, usize item_count, u64 id) {
	if (id == 0) {
		id = elf_hash((const u8*)&item, sizeof item);
	}

	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_style style = ui_get_style(ui, "combo", class, ui_style_variant_none);

	const v2f dimensions = v2f_add(make_v2f(ui->columns[ui->column] *
		container->rect.z - (style.padding.value.x * 2.0f + style.padding.value.z) - (container->padding.x + container->padding.z), get_font_height(ui->font)),
		make_v2f(0.0f, style.padding.value.y + style.padding.value.w));

	ui_draw_rect(ui, get_ui_el_position(ui, &style, dimensions), dimensions,
		style.background_colour.value, style.radius.value);
	struct ui_cmd_draw_rect* rect_cmd = ui_last_cmd(ui);

	bool hovered = container->interactable && mouse_over_rect(rect_cmd->position, dimensions);

	if (hovered) {
		ui->hovered = id;

		if (mouse_btn_just_released(mouse_btn_left)) {
			ui->active = id;
		}
	}

	bool active = ui->active == id;

	if (ui_get_style_variant(ui, &style, "combo", class, hovered, active)) {
		rect_cmd->position = get_ui_el_position(ui, &style, dimensions);
		rect_cmd->radius   = style.radius.value;

		rect_cmd->colour = style.background_colour.value;
	}

	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			rect_cmd->position,
			rect_cmd->dimensions,
			style.outline_colour.value, style.radius.value,
			style.outline_thickness.value);
	}

	if (item && *item >= 0) {
		const v2f text_pos = v2f_add(rect_cmd->position, style.padding.value);
		const v2f text_dimensions = get_text_dimensions(ui->font, items[*item]);
	
		ui_draw_text(ui, text_pos, text_dimensions, items[*item], style.text_colour.value);
	}

	bool changed = false;

	if (active) {
		v2f c = ui->cursor_pos;
		ui_begin_floating_container_ex(ui, "combo_container", make_v4f(
			rect_cmd->position.x,
			rect_cmd->position.y + rect_cmd->dimensions.y,
			rect_cmd->dimensions.x, 100.0f), true);

		for (usize i = 0; i < item_count; i++) {
			bool selected = false;
			ui_selectable_tree_node_ex(ui, "", items[i], true, &selected, id + i + 1);

			if (selected) {
				*item = (i32)i;
				changed = true;
			}
		}

		ui_end_container(ui);
		ui->cursor_pos = c;
	}

	ui_advance(ui, make_v2f(dimensions.x, dimensions.y + container->spacing));

	return changed;
}

bool ui_colour_picker_ex(struct ui* ui, const char* class, v4f* colour, u64 id) {
	if (id == 0) {
		id = elf_hash((const u8*)&colour, sizeof colour);
	}

	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_style style = ui_get_style(ui, "picker", class, ui_style_variant_none);

	f32 scale = get_dpi_scale();

	const v2f box_dimensions = make_v2f(
		lerp(style.min_size.value.x, style.max_size.value.x, 0.5f) * scale,
		lerp(style.min_size.value.y, style.max_size.value.y, 0.5f) * scale);
		
	const v2f result_dimensions = make_v2f(25.0f * scale, 25.0f * scale);

	const v2f slider_dimensions = make_v2f(15.0f * scale, box_dimensions.y);

	const v2f dimensions = make_v2f(box_dimensions.x + slider_dimensions.x + result_dimensions.x + style.padding.value.x * 2.0f, box_dimensions.y);
	const v2f position = ui->cursor_pos;
	const v2f slider_pos   = make_v2f(position.x + box_dimensions.x + style.padding.value.x, position.y);
	const v2f a_slider_pos = make_v2f(slider_pos.x + slider_dimensions.x + style.padding.value.x, position.y);
	const v2f result_pos   = make_v2f(a_slider_pos.x + slider_dimensions.x + style.padding.value.x, position.y);

	v4f hsva = rgba_to_hsva(*colour);

	const f32 slider_gradient_size = 0.166f;
	
	/* Hue slider. Made of multiple rectangles. */
	v4f hues[] = {
		make_rgba(0xff0000, 255),
		make_rgba(0xffff00, 255),
		make_rgba(0x00ff00, 255),
		make_rgba(0x00ffff, 255),
		make_rgba(0x0000ff, 255),
		make_rgba(0xff00ff, 255),
		make_rgba(0xff0000, 255)
	};

	for (i32 i = 0; i < 6; i++) {
		ui_draw_gradient(ui, make_v2f(slider_pos.x, slider_pos.y + slider_dimensions.y * slider_gradient_size * (f32)i),
			make_v2f(slider_dimensions.x, slider_dimensions.y * slider_gradient_size),
			hues[i], hues[i],
			hues[i + 1], hues[i + 1], 0.0f);
	}

	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			slider_pos,
			slider_dimensions,
			style.outline_colour.value, style.radius.value,
			style.outline_thickness.value);
	}

	/* Hue handle */
	if (container->interactable && mouse_over_rect(slider_pos, slider_dimensions) && mouse_btn_just_pressed(mouse_btn_left)) {
		ui->active = id + 2;
	}

	if (ui->active == id + 2) {
		v2i mouse_pos = get_mouse_pos();
		hsva.h = saturate(((f32)mouse_pos.y - slider_pos.y) / slider_dimensions.y);
	}

	const f32 hue_handle_y_pos = hsva.h * slider_dimensions.y;
	ui_draw_rect(ui,
		make_v2f(slider_pos.x, slider_pos.y + hue_handle_y_pos),
		make_v2f(slider_dimensions.x, 1.0f * scale), make_rgba(0x000000, 255), 0.0f);
	
	/* Alpha slider. */
	ui_draw_gradient(ui, a_slider_pos, slider_dimensions,
		make_rgba(0xffffff, 255), make_rgba(0xffffff, 255),
		make_rgba(0x000000, 255), make_rgba(0x000000, 255), style.radius.value);

	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			a_slider_pos,
			slider_dimensions,
			style.outline_colour.value, style.radius.value,
			style.outline_thickness.value);
	}

	/* Alpha handle */
	if (container->interactable && mouse_over_rect(a_slider_pos, slider_dimensions) && mouse_btn_just_pressed(mouse_btn_left)) {
		ui->active = id + 3;
	}

	if (ui->active == id + 3) {
		v2i mouse_pos = get_mouse_pos();
		hsva.a = 1.0f - saturate(((f32)mouse_pos.y - slider_pos.y) / slider_dimensions.y);
	}

	const f32 a_handle_y_pos = (1.0f - hsva.a) * slider_dimensions.y;
	ui_draw_rect(ui,
		make_v2f(a_slider_pos.x, a_slider_pos.y + a_handle_y_pos),
		make_v2f(slider_dimensions.x, 1.0f * scale), make_rgba(0x000000, 255), 0.0f);

	/* Saturation and Value control. The second rectangle dictates the brightness,
	 * the first dictates the hue. This is because the rasterizer's interpolation
	 * isn't really good enough as it produces visible triangles. */
	v4f hue_rgb = hsva_to_rgba((v4f) { hsva.h, 1.0f, 1.0f, 1.0f });
	ui_draw_gradient(ui, position, box_dimensions,
		make_rgba(0xffffff, 255), hue_rgb,
		make_rgba(0xffffff, 255), hue_rgb, style.radius.value);
	ui_draw_gradient(ui, position, box_dimensions,
		make_rgba(0x000000, 0),   make_rgba(0x000000, 0),
		make_rgba(0x000000, 255), make_rgba(0x000000, 255), style.radius.value);
	
	if (style.outline_thickness.value > 0.0f) {
		ui_draw_outline(ui,
			position,
			box_dimensions,
			style.outline_colour.value, style.radius.value,
			style.outline_thickness.value);
	}

	if (container->interactable && mouse_over_rect(position, box_dimensions) && mouse_btn_just_pressed(mouse_btn_left)) {
		ui->active = id + 1;
	}

	if (ui->active == id + 1) {
		v2i mouse_pos = get_mouse_pos();
		hsva.s =        saturate(((f32)mouse_pos.x - position.x - 2.5f * scale) / (box_dimensions.x));
		hsva.v = 1.0f - saturate(((f32)mouse_pos.y - position.y - 2.5f * scale) / (box_dimensions.y));
	}

	v2f handle_pos = make_v2f(hsva.s * box_dimensions.x, (1.0f - hsva.v) * box_dimensions.y);

	v2f handle_off = v2f_sub(position, make_v2f(2.5f, 2.5f));
	ui_draw_circle(ui, v2f_add(handle_off, handle_pos), 5.0f * scale, make_rgba(0x000000, 255));
	ui_draw_circle(ui, v2f_add(handle_off, v2f_add(handle_pos, make_v2f(1.0f * scale, 1.0f * scale))), 4.0f * scale, style.text_colour.value);
	
	v4f rgb_col = hsva_to_rgba(hsva);
	bool changed = false;
	if (!v4f_eq(rgb_col, *colour)) {
		*colour = rgb_col;
		changed = true;
	}

	/* Result. A checkerboard is drawn underneath to showcase transparency. */
	ui_draw_texture(ui, result_pos, result_dimensions, ui->alpha_texture, make_v4i(0, 0, 64, 64), make_rgba(0xffffff, 255), style.radius.value);
	ui_draw_rect(ui, result_pos, result_dimensions, *colour, style.radius.value);

	ui_advance(ui, make_v2f(dimensions.x, dimensions.y + container->spacing));

	return changed;
}

bool ui_anything_hovered(struct ui* ui) {
	return ui->hovered != 0;
}

void ui_grab_input(struct ui* ui) {
	ui->active = ui->last_input_id;
	ui->input_cursor = (u32)strlen(ui->last_input_buf);
}

void ui_draw(const struct ui* ui) {
#ifdef ui_print_commands
		info(" == UI Command Dump == ");
#endif

	for (i64 i = (i64)vector_count(ui->sorted_containers) - 1; i >= 0; i--) {
		struct ui_container_meta* meta = ui->sorted_containers[i];

#ifdef ui_print_commands
			info(" == Begin Container == ");
#endif

		for (usize j = 0; j < vector_count(meta->cmd_views); j++) {
			struct ui_cmd_view* view = &meta->cmd_views[j];

			struct ui_cmd* cmd = (void*)(((u8*)ui->cmd_buffer) + view->head);
			struct ui_cmd* end = (void*)(((u8*)ui->cmd_buffer) + view->tail);

			while (cmd != end) {
				switch (cmd->type) {
					case ui_cmd_draw_rect: {
						struct ui_cmd_draw_rect* rect = (struct ui_cmd_draw_rect*)cmd;

						ui_renderer_push(ui->renderer, &(struct ui_renderer_quad) {
							.position   = make_v2f(rect->position.x,   rect->position.y),
							.dimensions = make_v2f(rect->dimensions.x, rect->dimensions.y),
							.colour     = rect->colour,
							.radius     = rect->radius
						});

#ifdef ui_print_commands
						info("%llu\t\t%s", ((u8*)cmd) - ui->cmd_buffer, "rect");
#endif
					} break;
					case ui_cmd_draw_outline: {
						struct ui_cmd_draw_outline* outline = (struct ui_cmd_draw_outline*)cmd;

						ui_renderer_push(ui->renderer, &(struct ui_renderer_quad) {
							.position   = make_v2f(outline->position.x,   outline->position.y),
							.dimensions = make_v2f(outline->dimensions.x, outline->dimensions.y),
							.colour     = outline->colour,
							.radius     = outline->radius,
							.outline    = outline->thickness
						});

#ifdef ui_print_commands
						info("%llu\t\t%s", ((u8*)cmd) - ui->cmd_buffer, "outline");
#endif
					} break;
					case ui_cmd_draw_gradient: {
						struct ui_cmd_draw_gradient* grad = (struct ui_cmd_draw_gradient*)cmd;

						ui_renderer_push_gradient(ui->renderer, &(struct ui_renderer_gradient_quad) {
							.position = grad->position,
							.dimensions = grad->dimensions,
							.colours = {
								.top_left = grad->top_left,
								.top_right = grad->top_right,
								.bot_left = grad->bot_left,
								.bot_right = grad->bot_right
							},
							.radius = grad->radius
						});

#ifdef ui_print_commands
						info("%llu\t\t%s", ((u8*)cmd) - ui->cmd_buffer, "gradient");
#endif
					} break;
					case ui_cmd_draw_circle: {
						struct ui_cmd_draw_circle* circle = (struct ui_cmd_draw_circle*)cmd;

						v2f dimensions = make_v2f(circle->radius * 2.0f, circle->radius * 2.0f);

						ui_renderer_push(ui->renderer, &(struct ui_renderer_quad) {
							.position   = make_v2f(circle->position.x, circle->position.y),
							.dimensions = dimensions,
							.colour     = circle->colour,
							.radius     = circle->radius
						});

#ifdef ui_print_commands
						info("%llu\t\t%s", ((u8*)cmd) - ui->cmd_buffer, "circle");
#endif
					} break;
					case ui_cmd_draw_text: {
						struct ui_cmd_draw_text* text = (struct ui_cmd_draw_text*)cmd;

						ui_renderer_push_text(ui->renderer, &(struct ui_renderer_text) {
							.position = text->position,
							.text     = (char*)(text + 1),
							.colour   = text->colour,
							.font     = text->font
						});

#ifdef ui_print_commands
						info("%llu\t\t%s", ((u8*)cmd) - ui->cmd_buffer, "text");
#endif
					} break;
					case ui_cmd_clip: {
						struct ui_cmd_clip* clip = (struct ui_cmd_clip*)cmd;

						ui_renderer_clip(ui->renderer, clip->rect);

#ifdef ui_print_commands
						info("%llu\t\t%s", ((u8*)cmd) - ui->cmd_buffer, "clip");
#endif
					} break;
					case ui_cmd_draw_texture: {
						struct ui_cmd_texture* texture = (struct ui_cmd_texture*)cmd;

						ui_renderer_push(ui->renderer, &(struct ui_renderer_quad) {
							.position   = texture->position,
							.dimensions = texture->dimensions,
							.colour     = texture->colour,
							.rect       = make_v4f(
								(f32)texture->rect.x,
								(f32)texture->rect.y,
								(f32)texture->rect.z,
								(f32)texture->rect.w),
							.texture    = texture->texture,
							.radius     = texture->radius
						});

#ifdef ui_print_commands
						info("%llu\t\t%s", ((u8*)cmd) - ui->cmd_buffer, "texture");
#endif
					} break;
				}

				cmd = (void*)(((u8*)cmd) + cmd->size);
			}
		}
	}

	ui_renderer_flush(ui->renderer);
	ui_renderer_end_frame(ui->renderer);

	for (usize i = 0; i < vector_count(ui->cmd_free_queue); i++) {
		core_free(ui->cmd_free_queue[i]);
	}

	vector_clear(ui->cmd_free_queue);

#ifdef ui_print_commands
	info(" == End UI Command Dump == ");
#endif
}
