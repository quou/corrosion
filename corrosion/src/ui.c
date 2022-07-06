#include "core.h"
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
};

struct ui {
	struct ui_renderer* renderer;

	struct font* default_font;
	struct font* font;

	u8* cmd_buffer;
	usize cmd_buffer_idx;
	usize cmd_buffer_capacity;
	usize last_cmd_size;

	struct ui_stylesheet stylesheet;

	v2f cursor_pos;

	vector(struct ui_container) container_stack;

	u64 active_item;

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
			const struct ui_style* hovered_ptr = table_get(ui->stylesheet.hovered, class_id);

			if (hovered_ptr) {
				ui_build_style(dst, hovered_ptr);
			}
		} break;
		case ui_style_variant_active: {
			const struct ui_style* active_ptr = table_get(ui->stylesheet.active, class_id);

			if (active_ptr) {
				ui_build_style(dst, active_ptr);
			}
		} break;
		default: break;
	}
}

static struct ui_style ui_get_style(struct ui* ui, const char* base_class, const char* class, u32 variant) {
	const u64 base_name_hash = hash_string(base_class);
	const struct ui_style* base_ptr = table_get(ui->stylesheet.normal, base_name_hash);

	if (!base_ptr) {
		error("Base class `%s' not found in stylesheet.", base_class);
		return (struct ui_style) { 0 };
	}

	struct ui_style base = *base_ptr;
	ui_build_style_variant(ui, base_name_hash, &base, variant);

	const char* cur_class_start = class;
	usize cur_class_len = 0;
	for (const char* c = class; *c; c++) {
		if (*c == ' ' || *(c + 1) == '\0') {
			if (*c == ' ')        { cur_class_len--; }
			if (*(c + 1) == '\0') { cur_class_len++; }

			const u64 name_hash = elf_hash((const u8*)cur_class_start, cur_class_len);
			const struct ui_style* class_ptr = table_get(ui->stylesheet.normal, name_hash);
			if (!class_ptr) {
				error("Class `%.*s' not found in stylesheet.", cur_class_len, cur_class_start);
			} else {
				ui_build_style(&base, class_ptr);
				ui_build_style_variant(ui, name_hash, &base, variant);
			}

			cur_class_len = 0;
			cur_class_start = c + 1;
		} else {
			cur_class_len++;
		}
	}

	return base;
}

static bool ui_get_style_variant(struct ui* ui, struct ui_style* style, const char* base, const char* class, bool hovered) {
	if (hovered) {
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

struct ui* new_ui(const struct framebuffer* framebuffer) {
	struct ui* ui = core_calloc(1, sizeof(struct ui));

	ui->default_font = new_font(default_font, sizeof default_font, 14.0f);

	ui->renderer = new_ui_renderer(framebuffer);
	ui->cmd_buffer = core_alloc(1024);

	table_set(ui->stylesheet.normal, hash_string("label"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x111111, 255) },
		.padding           = { true, 0.0f },
		.align             = { true, ui_align_left },
	}));

	table_set(ui->stylesheet.normal, hash_string("button"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, 3.0f }
	}));

	table_set(ui->stylesheet.hovered, hash_string("button"), ((struct ui_style) {
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(ui->stylesheet.active, hash_string("button"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	table_set(ui->stylesheet.normal, hash_string("container"), ((struct ui_style) {
		.background_colour = { true, make_rgba(0x111111, 255) },
		.padding           = { true, 5.0f },
		.radius            = { true, 10.0f }
	}));

	table_set(ui->stylesheet.normal, hash_string("knob"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x191b26, 255) },
		.padding           = { true, 5.0f },
		.radius            = { true, 25.0f },
		.align             = { true, ui_align_centre }
	}));

	table_set(ui->stylesheet.hovered, hash_string("knob"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x252839, 255) },
	}));

	table_set(ui->stylesheet.active, hash_string("knob"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x000000, 255) },
		.background_colour = { true, make_rgba(0x8c91ac, 255) },
	}));

	return ui;
}

void free_ui(struct ui* ui) {
	free_vector(ui->container_stack);
	free_font(ui->default_font);
	free_ui_renderer(ui->renderer);
	core_free(ui->cmd_buffer);
	core_free(ui);
}

void ui_begin(struct ui* ui) {
	ui->cmd_buffer_idx = 0;

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
	ui->font = font ? font : ui->default_font;
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
	}
}

void ui_columns(struct ui* ui, usize count, f32* columns) {
	vector_clear(ui->columns);

	for (usize i = 0; i < count; i++) {
		vector_push(ui->columns, columns[i]);
	}

	ui->column_count = count;
	ui->column = 0;
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
	if (ui_get_style_variant(ui, &style, "label", class, hovered)) {
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

	const v2f dimentions = make_v2f(style.radius.value, style.radius.value);
	const v2f position = get_ui_el_position(ui, &style, dimentions);

	ui_draw_circle(ui, position, style.radius.value, style.background_colour.value);
	struct ui_cmd_draw_circle* knob_cmd = ui_last_cmd(ui);
	ui_draw_circle(ui, v2f_add(position, make_v2f(style.radius.value - 5.0f, 5.0f)), 5.0f, style.text_colour.value);
	struct ui_cmd_draw_circle* handle_cmd = ui_last_cmd(ui);

	bool hovered = mouse_over_circle(position, knob_cmd->radius);
	if (ui_get_style_variant(ui, &style, "knob", class, hovered)) {
		knob_cmd->radius   = style.radius.value;
		knob_cmd->position = get_ui_el_position(ui, &style, dimentions);
		knob_cmd->colour   = style.background_colour.value;

		handle_cmd->position = v2f_add(knob_cmd->position, make_v2f(style.radius.value - 5.0f, 5.0f));
		handle_cmd->colour = style.text_colour.value;
	}

	advance(ui, dimentions.y + container->padding);

	return false;
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
