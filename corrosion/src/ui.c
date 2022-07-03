#include "core.h"
#include "simplerenderer.h"
#include "ui.h"
#include "ui_atlas.h"

enum {
	ui_cmd_draw_rect,
	ui_cmd_draw_text,
	ui_cmd_clip
};

struct ui_cmd {
	u8 type;
	usize size;
};

struct ui_cmd_draw_rect {
	struct ui_cmd cmd;
	v4f rect;
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
};

struct ui_stylesheet {
	table(u64, struct ui_style) normal;
	table(u64, struct ui_style) hovered;
	table(u64, struct ui_style) active;
};

struct ui {
	struct simple_renderer* renderer;

	struct font* default_font;
	struct font* font;

	f32 padding;

	u8* cmd_buffer;
	usize cmd_buffer_idx;
	usize cmd_buffer_capacity;
	usize last_cmd_size;

	struct ui_stylesheet stylesheet;

	v2f cursor_pos;

	vector(struct ui_container) container_stack;

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

static void ui_draw_rect(struct ui* ui, v4f rect, v4f colour) {
	struct ui_cmd_draw_rect* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_rect));
	cmd->cmd.type = ui_cmd_draw_rect;
	cmd->cmd.size = sizeof *cmd;
	cmd->rect = rect;
	cmd->colour = colour;
}

static v2f ui_draw_text(struct ui* ui, v2f position, const char* text, v4f colour) {
	usize len = strlen(text);

	struct ui_cmd_draw_text* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_draw_text) + len + 1);
	cmd->cmd.type = ui_cmd_draw_text;
	cmd->cmd.size = (sizeof *cmd) + len + 1;
	cmd->position = position;
	cmd->colour = colour;
	cmd->font = ui->font;

	memcpy(cmd + 1, text, len);
	((char*)(cmd + 1))[len] = '\0';

	return get_text_dimentions(cmd->font, text);
}

static void ui_clip(struct ui* ui, v4f rect) {
	struct ui_cmd_clip* cmd = ui_cmd_add(ui, sizeof(struct ui_cmd_clip));
	cmd->cmd.type = ui_cmd_clip;
	cmd->cmd.size = sizeof *cmd;
	cmd->rect = make_v4i((f32)rect.x, (f32)rect.y, (f32)rect.z, (f32)rect.w);
}

static inline void ui_build_style(struct ui_style* dst, const struct ui_style* src) {
	if (src->text_colour.has_value)       { dst->text_colour       = src->text_colour; }
	if (src->background_colour.has_value) { dst->background_colour = src->background_colour; }
	if (src->align.has_value)             { dst->align             = src->align; }
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
		if (*c == ' ') {
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

struct ui* new_ui(const struct framebuffer* framebuffer) {
	struct ui* ui = core_calloc(1, sizeof(struct ui));

	ui->padding = 5.0f;

	ui->default_font = new_font(default_font, sizeof default_font, 14.0f);

	ui->renderer = new_simple_renderer(framebuffer);
	ui->cmd_buffer = core_alloc(1024);

	table_set(ui->stylesheet.normal, hash_string("label"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xffffff, 255) },
		.background_colour = { true, make_rgba(0x111111, 255) },
		.align             = { true, ui_align_left },
	}));

	table_set(ui->stylesheet.hovered, hash_string("label"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0xff0000, 255) },
	}));

	table_set(ui->stylesheet.active, hash_string("label"), ((struct ui_style) {
		.text_colour       = { true, make_rgba(0x00ff00, 255) },
	}));

	table_set(ui->stylesheet.normal, hash_string("container"), ((struct ui_style) {
		.background_colour = { true, make_rgba(0x111111, 255) },
	}));

	return ui;
}

void free_ui(struct ui* ui) {
	free_vector(ui->container_stack);
	free_font(ui->default_font);
	free_simple_renderer(ui->renderer);
	core_free(ui->cmd_buffer);
	core_free(ui);
}

void ui_begin(struct ui* ui) {
	ui->cmd_buffer_idx = 0;

	ui_font(ui, null);

	v2i window_size = get_window_size();
	simple_renderer_clip(ui->renderer, make_v4i(0, 0, window_size.x, window_size.y));

	ui->cursor_pos = make_v2f(0.0f, 0.0f);

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

	v4f clip;
	if (parent) {
		clip = make_v4f(
			rect.x * (f32)(parent->rect.z) + ui->padding + parent->rect.x,
			rect.y * (f32)(parent->rect.w) + ui->padding + parent->rect.y,
			rect.z * (f32)(parent->rect.z - parent->rect.x) - ui->padding * 2.0f,
			rect.w * (f32)(parent->rect.w - parent->rect.y) - ui->padding * 2.0f);

		ui_clip(ui, clip);
		const struct ui_style style = ui_get_style(ui, "container", class, ui_style_variant_none);
		ui_draw_rect(ui, clip, style.background_colour.value);
	} else {
		clip = make_v4f(
			rect.x * (f32)window_size.x,
			rect.y * (f32)window_size.y,
			rect.z * (f32)window_size.x,
			rect.w * (f32)window_size.y);
		ui_clip(ui, clip);
	}

	vector_push(ui->container_stack, ((struct ui_container) {
		.rect = clip
	}));

	ui->cursor_pos = make_v2f(clip.x + ui->padding, clip.y + ui->padding);
}

void ui_end_container(struct ui* ui) {
	vector_pop(ui->container_stack);
	ui->cursor_pos.x -= ui->padding;
	ui->cursor_pos.y += ui->padding;
}

void ui_font(struct ui* ui, struct font* font) {
	ui->font = font ? font : ui->default_font;
}

static v2f get_ui_el_position(struct ui* ui, const struct ui_style* style, v2f dimentions) {
	const struct ui_container* container = vector_end(ui->container_stack) - 1;

	switch (style->align.value) {
		case ui_align_left:
			return ui->cursor_pos;
		case ui_align_right:
			return make_v2f((container->rect.x + container->rect.z) - dimentions.x - ui->padding, ui->cursor_pos.y);
		case ui_align_centre:
			return make_v2f((container->rect.x + container->rect.z * 0.5f) - (dimentions.x * 0.5f), ui->cursor_pos.y);
			break;
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
		ui->cursor_pos.x = container->rect.x + ui->padding;
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

	const v2f dimentions = ui_draw_text(ui, ui->cursor_pos, text, make_rgba(0xffffff, 255));
	struct ui_cmd_draw_text* cmd = ui_last_cmd(ui);

	struct ui_style style = ui_get_style(ui, "label", class, ui_style_variant_none);
	cmd->position = get_ui_el_position(ui, &style, dimentions);

	bool hovered = mouse_over_rect(cmd->position, dimentions); 

	if (hovered) {
		if (mouse_btn_pressed(mouse_btn_left)) {
			style = ui_get_style(ui, "label", class, ui_style_variant_active);
		} else {
			style = ui_get_style(ui, "label", class, ui_style_variant_hovered);
		}
	}

	cmd->colour = style.text_colour.value;
	cmd->position = get_ui_el_position(ui, &style, dimentions);

	advance(ui, dimentions.y + ui->padding);

	if (hovered && mouse_btn_just_released(mouse_btn_left)) { return true; }
	return false;
}

void ui_begin_window() {

}

void ui_end_window() {

}

void ui_draw(const struct ui* ui) {
	struct ui_cmd* cmd = (void*)ui->cmd_buffer;
	struct ui_cmd* end = (void*)(((u8*)ui->cmd_buffer) + ui->cmd_buffer_idx);

	while (cmd != end) {
		switch (cmd->type) {
			case ui_cmd_draw_rect: {
				struct ui_cmd_draw_rect* rect = (struct ui_cmd_draw_rect*)cmd;
				simple_renderer_push(ui->renderer, &(struct simple_renderer_quad) {
					.position   = make_v2f(rect->rect.x, rect->rect.y),
					.dimentions = make_v2f(rect->rect.z, rect->rect.w),
					.colour     = rect->colour
				});
			} break;
			case ui_cmd_draw_text: {
				struct ui_cmd_draw_text* text = (struct ui_cmd_draw_text*)cmd;
				simple_renderer_push_text(ui->renderer, &(struct simple_renderer_text) {
					.position = text->position,
					.text     = (char*)(text + 1),
					.colour   = text->colour,
					.font     = text->font
				});
			} break;
			case ui_cmd_clip: {
				struct ui_cmd_clip* clip = (struct ui_cmd_clip*)cmd;
				simple_renderer_clip(ui->renderer, clip->rect);
			} break;
		}

		cmd = (void*)(((u8*)cmd) + cmd->size);
	}

	simple_renderer_flush(ui->renderer);
	simple_renderer_end_frame(ui->renderer);
}
