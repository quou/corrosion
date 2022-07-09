#include "core.h"
#include "dtable.h"
#include "ui.h"
#include "ui_atlas.h"
#include "ui_render.h"

enum {
	ui_cmd_draw_rect,
	ui_cmd_draw_text,
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
	v2f dimentions;
	v4f colour;
	f32 radius;
};

struct ui_cmd_draw_circle {
	struct ui_cmd cmd;
	v2f position;
	f32 radius;
	v4f colour;
};

struct ui_cmd_draw_text {
	struct ui_cmd cmd;
	struct font* font;
	v2f position;
	v4f colour;
};

struct ui_cmd_clip {
	struct ui_cmd cmd;
	v4i rect;
};

struct ui_container {
	v4f rect;
	f32 padding;
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
	optional(u32) align;
	optional(f32) padding;
	optional(f32) radius;
};

struct ui_stylesheet {
	table(u64, struct ui_style) normal;
	table(u64, struct ui_style) hovered;
	table(u64, struct ui_style) active;
} default_stylesheet;

struct font* default_font;

struct ui {
	struct ui_renderer* renderer;

	struct font* default_font;
	struct font* font;

	u8* cmd_buffer;
	usize cmd_buffer_idx;
	usize cmd_buffer_capacity;
	usize last_cmd_size;

	struct ui_stylesheet* stylesheet;

	v2f cursor_pos;
	v4f current_clip;

	vector(struct ui_container) container_stack;

	char* temp_str;
	usize temp_str_size;

	u64 active;
	u64 dragging;
	u64 hovered;

	u32 input_cursor;

	f32 knob_angle_offset;

	usize column_count;
	usize column;
	f32 current_item_height;
	vector(f32) columns;
};

static inline bool mouse_over_rect(v2f position, v2f dimentions) {
	v2i mouse_pos = get_mouse_pos();

	return
		mouse_pos.x > (i32)position.x &&
		mouse_pos.y > (i32)position.y &&
		mouse_pos.x < (i32)position.x + dimentions.x &&
		mouse_pos.y < (i32)position.y + dimentions.y;
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
		ui->cmd_buffer_capacity += 1024;
		ui->cmd_buffer = core_realloc(ui->cmd_buffer, ui->cmd_buffer_capacity);
	}

	ui->last_cmd_size = size;
	ui->cmd_buffer_idx += size;

	return ui->cmd_buffer + (ui->cmd_buffer_idx - size);
}

void ui_draw_rect(struct ui* ui, v2f position, v2f dimentions, v4f colour, f32 radius) {
	struct ui_cmd_draw_rect* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_rect));
	cmd->cmd.type = ui_cmd_draw_rect;
	cmd->cmd.size = sizeof *cmd;
	cmd->position = position;
	cmd->dimentions = dimentions;
	cmd->colour = colour;
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

void ui_draw_text(struct ui* ui, v2f position, const char* text, v4f colour) {
	usize len = strlen(text);

	struct ui_cmd_draw_text* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_text) + len + 1);
	cmd->cmd.type = ui_cmd_draw_text;
	cmd->cmd.size = (sizeof *cmd) + len + 1;
	cmd->position = position;
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

static inline void ui_build_style(struct ui_style* dst, const struct ui_style* src) {
	if (src->text_colour.has_value)       { dst->text_colour       = src->text_colour; }
	if (src->background_colour.has_value) { dst->background_colour = src->background_colour; }
	if (src->align.has_value)             { dst->align             = src->align; }
	if (src->padding.has_value)           { dst->padding           = src->padding; }
	if (src->radius.has_value)            { dst->radius            = src->radius; }
}

static inline void ui_build_style_variant(struct ui* ui, u64 class_id, struct ui_style* dst, u32 variant) {
	switch (variant) {
		case ui_style_variant_hovered: {
			const struct ui_style* hovered_ptr = table_get(ui->stylesheet->hovered, class_id);

			if (hovered_ptr) {
				ui_build_style(dst, hovered_ptr);
			}
		} break;
		case ui_style_variant_active: {
			const struct ui_style* active_ptr = table_get(ui->stylesheet->active, class_id);

			if (active_ptr) {
				ui_build_style(dst, active_ptr);
			}
		} break;
		default: break;
	}
}

static struct ui_style ui_get_style(struct ui* ui, const char* base_class, const char* class, u32 variant) {
	const u64 base_name_hash = hash_string(base_class);
	const struct ui_style* base_ptr = table_get(ui->stylesheet->normal, base_name_hash);

	if (!base_ptr) {
		error("Base class `%s' not found in stylesheet.", base_class);
		return (struct ui_style) { 0 };
	}

	struct ui_style base = *base_ptr;
	ui_build_style_variant(ui, base_name_hash, &base, variant);

	usize class_name_size = strlen(class) + 1;
	if (class_name_size > ui->temp_str_size) {
		ui->temp_str = core_realloc(ui->temp_str, class_name_size);
		ui->temp_str_size = class_name_size;
	}

	memcpy(ui->temp_str, class, class_name_size);

	char* cur_class = strtok(ui->temp_str, " ");
	while (cur_class) {
		const u64 name_hash = hash_string(cur_class);
		const struct ui_style* class_ptr = table_get(ui->stylesheet->normal, name_hash);
		if (!class_ptr) {
			error("Class `%s' not found in stylesheet.", cur_class);
		} else {
			ui_build_style(&base, class_ptr);
			ui_build_style_variant(ui, name_hash, &base, variant);
		}

		cur_class = strtok(null, " ");
	}

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
	table(u64, struct ui_style)* dst = dst_vptr;

	for (usize i = 0; i < vector_count(src->children); i++) {
		struct dtable* class = src->children + i;

		struct ui_style s = { 0 };
		struct ui_style* got = table_get(*dst, class->key.hash);
		if (got) { s = *got; }

		for (usize j = 0; j < vector_count(class->children); j++) {
			struct dtable* child = class->children + j;

			switch (child->value.type) {
				case dtable_colour:
					if (child->key.hash == hash_string("text_colour")) {
						s.text_colour.has_value = true;
						s.text_colour.value = child->value.as.colour;
					} else if (child->key.hash == hash_string("background_colour")) {
						s.background_colour.has_value = true;
						s.background_colour.value = child->value.as.colour;
					}
					break;
				case dtable_number:
					if (child->key.hash == hash_string("padding")) {	
						s.padding.has_value = true;
						s.padding.value = (f32)child->value.as.number;
					} else if (child->key.hash == hash_string("radius")) {
						s.radius.has_value = true;
						s.radius.value = (f32)child->value.as.number;
					}
				case dtable_string:
					if (child->key.hash == hash_string("align")) {
						s.align.has_value = true;
						s.align.value = strcmp(child->value.as.string, "left") == 0 ? ui_align_left :
							strcmp(child->value.as.string, "centre") == 0 ? ui_align_centre :
							ui_align_right;
					}
					break;
			}
		}

		table_set(*dst, class->key.hash, s);
	}
}

static void stylesheet_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	struct ui_stylesheet* stylesheet = payload;
	for (u64* i = table_first(default_stylesheet.normal); i; i = table_next(default_stylesheet.normal, *i)) {
		table_set(stylesheet->normal, *i, *(struct ui_style*)table_get(default_stylesheet.normal, *i));
	}

	for (u64* i = table_first(default_stylesheet.active); i; i = table_next(default_stylesheet.active, *i)) {
		table_set(stylesheet->active, *i, *(struct ui_style*)table_get(default_stylesheet.active, *i));
	}

	for (u64* i = table_first(default_stylesheet.hovered); i; i = table_next(default_stylesheet.hovered, *i)) {
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
	table_set(default_stylesheet.normal, hash_string("label"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x111111, 0) },
		.padding           = { true, 0.0f },
		.align             = { true, ui_align_left },
	}));

	table_set(default_stylesheet.normal, hash_string("button"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, 3.0f }
	}));

	table_set(default_stylesheet.hovered, hash_string("button"), ((struct ui_style) {
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, hash_string("button"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, hash_string("input"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, 3.0f }
	}));

	table_set(default_stylesheet.hovered, hash_string("input"), ((struct ui_style) {
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, hash_string("input"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(default_stylesheet.normal, hash_string("container"), ((struct ui_style) {
		.background_colour = { true, make_rgba(0x111111, 255) },
		.padding           = { true, 5.0f },
		.radius            = { true, 10.0f }
	}));

	table_set(default_stylesheet.normal, hash_string("knob"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, 5.0f },
		.radius            = { true, 25.0f },
		.align             = { true, ui_align_centre }
	}));

	table_set(default_stylesheet.hovered, hash_string("knob"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(default_stylesheet.active, hash_string("knob"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	reg_res_type("stylesheet", &(struct res_config) {
		.payload_size = sizeof(struct ui_stylesheet),
		.free_raw_on_load = true,
		.terminate_raw = true,
		.on_load = stylesheet_on_load,
		.on_unload = stylesheet_on_unload
	});

	default_font = new_font(default_font_atlas, sizeof default_font_atlas, 14.0f);
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

	return ui;
}

void free_ui(struct ui* ui) {
	core_free(ui->temp_str);
	free_vector(ui->columns);
	free_vector(ui->container_stack);
	free_ui_renderer(ui->renderer);
	core_free(ui->cmd_buffer);
	core_free(ui);
}

void ui_begin(struct ui* ui) {
	ui->cmd_buffer_idx = 0;

	ui->hovered = 0;

	ui_font(ui, null);

	v2i window_size = get_window_size();
	ui_renderer_clip(ui->renderer, make_v4i(0, 0, window_size.x, window_size.y));

	ui->cursor_pos = make_v2f(0.0f, 0.0f);
	ui->current_item_height = 0.0f;

	ui_begin_container(ui, make_v4f(0.0f, 0.0f, 1.0f, 1.0f));

	ui_columns(ui, 1, (f32[]) { 1.0f });
}

void ui_end(struct ui* ui) {
	ui_end_container(ui);

	if (mouse_btn_just_released(mouse_btn_left)) {
		ui->dragging = 0;

		if (!ui->hovered) {
			ui->active = 0;
		}
	}
}

void ui_begin_container_ex(struct ui* ui, const char* class, v4f rect) {
	struct ui_container* parent = null;
	if (vector_count(ui->container_stack) > 0) {
		parent = vector_end(ui->container_stack) - 1;
	}

	v2i window_size = get_window_size();

	f32 pad_top_bottom = 0.0f;
	f32 padding = 0.0f;
	v4f clip;
	if (parent) {
		const struct ui_style style = ui_get_style(ui, "container", class, ui_style_variant_none);
		pad_top_bottom = parent->padding;
		padding = style.padding.value;

		clip = make_v4f(
			rect.x * (f32)(parent->rect.z) + style.padding.value + parent->rect.x,
			rect.y * (f32)(parent->rect.w) + style.padding.value + parent->rect.y,
			rect.z * (f32)(parent->rect.z - parent->rect.x) - style.padding.value * 2.0f,
			rect.w * (f32)(parent->rect.w - parent->rect.y) - style.padding.value * 2.0f);

		ui_clip(ui, clip);
		ui_draw_rect(ui, make_v2f(clip.x, clip.y), make_v2f(clip.z, clip.w), style.background_colour.value, style.radius.value);
	} else {
		clip = make_v4f(
			rect.x * (f32)window_size.x,
			rect.y * (f32)window_size.y,
			rect.z * (f32)window_size.x,
			rect.w * (f32)window_size.y);
		ui_clip(ui, clip);
	}

	vector_push(ui->container_stack, ((struct ui_container) {
		.rect = clip,
		.padding = padding
	}));

	ui->cursor_pos = make_v2f(clip.x + padding, clip.y + padding);
}

void ui_end_container(struct ui* ui) {
	struct ui_container* container = vector_pop(ui->container_stack);
	ui->cursor_pos.x -= container->padding;
	ui->cursor_pos.y += container->padding;
}

void ui_font(struct ui* ui, struct font* font) {
	ui->font = font ? font : default_font;
}

void ui_stylesheet(struct ui* ui, struct ui_stylesheet* ss) {
	ui->stylesheet = ss ? ss : &default_stylesheet;
}

static v2f get_ui_el_position(struct ui* ui, const struct ui_style* style, v2f dimentions) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	f32 x_margin = 0.0f;
	for (usize i = 0; i < ui->column + 1; i++) {
		x_margin += ui->columns[i] * container->rect.z;
	}

	switch (style->align.value) {
		case ui_align_left:
			return ui->cursor_pos;
		case ui_align_right:
			return make_v2f((container->rect.x + x_margin) - dimentions.x - container->padding, ui->cursor_pos.y);
		case ui_align_centre: {
			f32 x_inner_margin = x_margin - ui->columns[ui->column] * container->rect.z;
			return make_v2f((container->rect.x + x_inner_margin * 0.5f + x_margin * 0.5f) - (dimentions.x * 0.5f), ui->cursor_pos.y);
		}
		default:
			return make_v2f(0.0f, 0.0f);
	}
}

static void advance(struct ui* ui, f32 last_height) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	if (last_height > ui->current_item_height) {
		ui->current_item_height = last_height;
	}

	ui->cursor_pos.x += ui->columns[ui->column] * container->rect.z;
	ui->column++;

	if (ui->column >= ui->column_count) {
		ui->cursor_pos.x = container->rect.x + container->padding;
		ui->cursor_pos.y += ui->current_item_height;
		ui->column = 0;
		ui->current_item_height = 0.0f;
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

bool ui_label_ex(struct ui* ui, const char* class, const char* text) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_style style = ui_get_style(ui, "label", class, ui_style_variant_none);

	const v2f dimentions = v2f_add(get_text_dimentions(ui->font, text), make_v2f(style.padding.value * 2.0f, style.padding.value * 2.0f));

	ui_draw_rect(ui, get_ui_el_position(ui, &style, dimentions), make_v2f(dimentions.x, dimentions.y),
		style.background_colour.value, style.radius.value);
	struct ui_cmd_draw_rect* rect_cmd = ui_last_cmd(ui);

	ui_draw_text(ui, v2f_add(rect_cmd->position, make_v2f(style.padding.value, style.padding.value)), text, style.text_colour.value);
	struct ui_cmd_draw_text* text_cmd = ui_last_cmd(ui);

	bool hovered = mouse_over_rect(rect_cmd->position, dimentions);
	if (ui_get_style_variant(ui, &style, "label", class, hovered, false)) {
		rect_cmd->position = get_ui_el_position(ui, &style, dimentions);
		rect_cmd->radius   = style.radius.value;
		text_cmd->position = v2f_add(rect_cmd->position, make_v2f(style.padding.value, style.padding.value));

		rect_cmd->colour = style.background_colour.value;
		text_cmd->colour = style.text_colour.value;
	}

	advance(ui, dimentions.y + container->padding);

	if (hovered && mouse_btn_just_released(mouse_btn_left)) { return true; }
	return false;
}

bool ui_knob_ex(struct ui* ui, const char* class, f32* val, f32 min, f32 max) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	const u64 id = elf_hash((const u8*)&val, sizeof val);

	struct ui_style style = ui_get_style(ui, "knob", class, ui_style_variant_none);

	const f32 handle_radius = 5.0f;

	const v2f dimentions = make_v2f(style.radius.value * 2.0f, style.radius.value * 2.0f);
	const v2f position = get_ui_el_position(ui, &style, dimentions);

	ui_draw_circle(ui, position, style.radius.value, style.background_colour.value);
	struct ui_cmd_draw_circle* knob_cmd = ui_last_cmd(ui);
	ui_draw_circle(ui, make_v2f(0.0f, 0.0f), handle_radius, style.text_colour.value);
	struct ui_cmd_draw_circle* handle_cmd = ui_last_cmd(ui);

	bool hovered = mouse_over_circle(position, knob_cmd->radius);

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

		knob_cmd->radius   = style.radius.value;
		knob_cmd->position = get_ui_el_position(ui, &style, dimentions);
		knob_cmd->colour   = style.background_colour.value;

		handle_cmd->colour = style.text_colour.value;

		knob_origin = make_v2f(
			knob_cmd->position.x + style.radius.value - handle_radius,
			knob_cmd->position.y + style.radius.value - handle_radius);

		v2i mouse_pos = get_mouse_pos();
		v2f v = v2f_sub(make_v2f(mouse_pos.x, mouse_pos.y), knob_origin);
		angle = atan2f(-v.x, -v.y) + to_rad(180.0f);

		angle += ui->knob_angle_offset;
		angle = clamp(angle, min_angle, max_angle);

		*val = map(angle, min_angle, max_angle, max, min);
		*val = clamp(*val, min, max);

		changed = true;
	} else if (ui_get_style_variant(ui, &style, "knob", class, hovered, false)) {
		knob_cmd->radius   = style.radius.value;
		knob_cmd->position = get_ui_el_position(ui, &style, dimentions);
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
			make_v2f(style.radius.value - handle_radius - style.padding.value, style.radius.value - handle_radius - style.padding.value)));

	advance(ui, dimentions.y + container->padding);

	return changed;
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

bool ui_input_ex2(struct ui* ui, const char* class, char* buf, usize buf_size, ui_input_filter filter) {
	const u64 id = elf_hash((const u8*)&buf, sizeof buf);

	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	struct ui_style style = ui_get_style(ui, "input", class, ui_style_variant_none);

	const v2f dimentions = v2f_add(make_v2f(ui->columns[ui->column] * container->rect.z, get_font_height(ui->font)),
		make_v2f(style.padding.value * 2.0f, style.padding.value * 2.0f));
	const f32 cursor_x_off = get_text_n_dimentions(ui->font, buf, ui->input_cursor).x;

	ui_draw_rect(ui, get_ui_el_position(ui, &style, dimentions), make_v2f(dimentions.x, dimentions.y),
		style.background_colour.value, style.radius.value);
	struct ui_cmd_draw_rect* rect_cmd = ui_last_cmd(ui);

	struct ui_cmd_draw_rect* cursor_cmd = null;

	bool hovered = mouse_over_rect(rect_cmd->position, dimentions);

	if (hovered) {
		ui->hovered = id;

		if (mouse_btn_just_released(mouse_btn_left)) {
			ui->active = id;
			ui->input_cursor = strlen(buf);
		}
	}

	bool active = ui->active == id;

	if (ui_get_style_variant(ui, &style, "input", class, hovered, active)) {
		rect_cmd->position = get_ui_el_position(ui, &style, dimentions);
		rect_cmd->radius   = style.radius.value;

		rect_cmd->colour = style.background_colour.value;
	}

	const v2f text_pos = v2f_add(rect_cmd->position, make_v2f(style.padding.value, style.padding.value));

	bool submitted = false;

	v4f prev_clip = ui->current_clip;
	ui_clip(ui, make_v4f(text_pos.x, text_pos.y, dimentions.x - style.padding.value, dimentions.y - style.padding.value));

	f32 scroll = 0.0f;
	if (active) {
		scroll = min(dimentions.x - cursor_x_off - style.padding.value - get_font_height(ui->font), 0.0f);

		ui_draw_rect(ui, v2f_add(text_pos, make_v2f(cursor_x_off + scroll, 0.0f)), make_v2f(1.0f, get_font_height(ui->font)),
			style.text_colour.value, 0.0f);
		cursor_cmd = ui_last_cmd(ui);

		usize input_len;
		const char* input_string;

		usize buf_len = strlen(buf);

		if (get_input_string(&input_string, &input_len) && buf_len + input_len < buf_size) {
			for (usize i = 0; i < input_len; i++) {
				if (filter(input_string[i])) {
					strncat(buf, input_string + i, 1);
					ui->input_cursor++;
				}
			}
		}

		if (key_just_pressed(key_return)) {
			submitted = true;
			ui->active = 0;
			active = false;
		}

		if (ui->input_cursor > 0 && key_just_pressed(key_backspace)) {
			for (u32 i = ui->input_cursor - 1; i < buf_len - 1; i++) {
				buf[i] = buf[i + 1];
			}

			buf[buf_len - 1] = '\0';
			ui->input_cursor--;
		}
	}

	ui_draw_text(ui, make_v2f(text_pos.x + scroll, text_pos.y), buf, style.text_colour.value);
	struct ui_cmd_draw_text* text_cmd = ui_last_cmd(ui);

	ui_clip(ui, prev_clip);

	advance(ui, dimentions.y + container->padding);

	return submitted;
}

void ui_draw(const struct ui* ui) {
	struct ui_cmd* cmd = (void*)ui->cmd_buffer;
	struct ui_cmd* end = (void*)(((u8*)ui->cmd_buffer) + ui->cmd_buffer_idx);

	while (cmd != end) {
		switch (cmd->type) {
			case ui_cmd_draw_rect: {
				struct ui_cmd_draw_rect* rect = (struct ui_cmd_draw_rect*)cmd;
				ui_renderer_push(ui->renderer, &(struct ui_renderer_quad) {
					.position   = make_v2f(rect->position.x,   rect->position.y),
					.dimentions = make_v2f(rect->dimentions.x, rect->dimentions.y),
					.colour     = rect->colour,
					.radius     = rect->radius
				});
			} break;
			case ui_cmd_draw_circle: {
				struct ui_cmd_draw_circle* circle = (struct ui_cmd_draw_circle*)cmd;
				ui_renderer_push(ui->renderer, &(struct ui_renderer_quad) {
					.position   = make_v2f(circle->position.x, circle->position.y),
					.dimentions = make_v2f(circle->radius * 2.0f, circle->radius * 2.0f),
					.colour     = circle->colour,
					.radius     = circle->radius
				});
			} break;
			case ui_cmd_draw_text: {
				struct ui_cmd_draw_text* text = (struct ui_cmd_draw_text*)cmd;
				ui_renderer_push_text(ui->renderer, &(struct ui_renderer_text) {
					.position = text->position,
					.text     = (char*)(text + 1),
					.colour   = text->colour,
					.font     = text->font
				});
			} break;
			case ui_cmd_clip: {
				struct ui_cmd_clip* clip = (struct ui_cmd_clip*)cmd;
				ui_renderer_clip(ui->renderer, clip->rect);
			} break;
		}

		cmd = (void*)(((u8*)cmd) + cmd->size);
	}

	ui_renderer_flush(ui->renderer);
	ui_renderer_end_frame(ui->renderer);
}
