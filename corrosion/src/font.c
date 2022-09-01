#include <ctype.h>

#include "core.h"
#include "font.h"
#include "stb.h"

#define glyphset_max 256

struct glyph_set {
	struct texture* atlas;
	stbtt_bakedchar glyphs[glyphset_max];
};

struct font {
	const void* data;
	stbtt_fontinfo info;
	struct glyph_set* sets[glyphset_max];
	f32 size;
	i32 height;
};

static const char* utf8_to_codepoint(const char* p, u32* dst) {
	u32 res, n;
	switch (*p & 0xf0) {
		case 0xf0 : res = *p & 0x07; n = 3; break;
		case 0xe0 : res = *p & 0x0f; n = 2; break;
		case 0xd0 :
		case 0xc0 : res = *p & 0x1f; n = 1; break;
		default   : res = *p;        n = 0; break;
	}
	while (n--) {
		res = (res << 6) | (*(++p) & 0x3f);
	}
	*dst = res;
	return p + 1;
}

static struct glyph_set* load_glyph_set(struct font* font, i32 idx) {
	struct glyph_set* set = core_calloc(1, sizeof(struct glyph_set));

	struct image image;

	image.size.x = 512;
	image.size.y = 512;
retry:
	image.colours = core_alloc(image.size.x * image.size.y * 4);

	f32 s = stbtt_ScaleForMappingEmToPixels(&font->info, 1) /
		stbtt_ScaleForPixelHeight(&font->info, 1);
	i32 r = stbtt_BakeFontBitmap(font->data, 0, font->size * s,
			image.colours, image.size.x, image.size.y, idx * 256, 256, set->glyphs);

	if (r <= 0) {
		image.size.x *= 2;
		image.size.y *= 2;
		core_free(image.colours);
		goto retry;
	}

	i32 ascent, descent, linegap, scaled_ascent;
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &linegap);
	f32 scale = stbtt_ScaleForMappingEmToPixels(&font->info, font->size);
	scaled_ascent = (i32)(ascent * scale + 0.5);
	for (u32 i = 0; i < 256; i++) {
		set->glyphs[i].yoff += scaled_ascent;
		set->glyphs[i].xadvance = (f32)floor(set->glyphs[i].xadvance);
	}

	for (i32 i = image.size.x * image.size.y - 1; i >= 0; i--) {
		unsigned char n = *((u8*)image.colours + i);
		image.colours[(i * 4) + 0] = 255;
		image.colours[(i * 4) + 1] = 255;
		image.colours[(i * 4) + 2] = 255;
		image.colours[(i * 4) + 3] = n;
	}

	set->atlas = video.new_texture(&image, texture_flags_filter_none, texture_format_rgba8i);

	core_free(image.colours);

	return set;
}

static struct glyph_set* get_glyph_set(struct font* font, i32 codepoint) {
	i32 idx = (codepoint >> 8) % glyphset_max;
	if (!font->sets[idx]) {
		font->sets[idx] = load_glyph_set(font, idx);
	}
	return font->sets[idx];
}

usize font_struct_size() {
	return sizeof(struct font);
}

struct font* new_font(const u8* raw, usize raw_size, f32 size) {
	struct font* font = core_calloc(1, sizeof(struct font));

	init_font(font, raw, raw_size, size);

	return font;
}

void free_font(struct font* font) {
	deinit_font(font);

	core_free(font);
}

void init_font(struct font* font, const u8* raw, usize raw_size, f32 size) {
	memset(font, 0, sizeof *font);

	font->data = raw;

	font->size = size;

	i32 r = stbtt_InitFont(&font->info, font->data, 0);
	if (!r) {
		goto fail;
	}

	i32 ascent, descent, linegap;
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &linegap);
	f32 scale = stbtt_ScaleForMappingEmToPixels(&font->info, size);
	font->height = (i32)((ascent - descent + linegap) * scale + 0.5);

	stbtt_bakedchar* g = get_glyph_set(font, '\n')->glyphs;
	g['\t'].x1 = g['\t'].x0;
	g['\n'].x1 = g['\n'].x0;

	return;

fail:
	if (font) {
		core_free(font);
	}
}

void deinit_font(struct font* font) {
	for (usize i = 0; i < glyphset_max; i++) {
		struct glyph_set* set = font->sets[i];
		if (set) {
			video.free_texture(set->atlas);
			core_free(set);
		}
	}
}

void* get_font_data(struct font* font) {
	return (void*)font->data;
}

void render_text(const struct text_renderer* renderer, struct font* font, const char* text, v2f position, v4f colour) {
	f32 x = position.x;
	f32 y = position.y;

	f32 ori_x = x;

	const char* p = text;
	while (*p) {
		if (*p == '\n') {
			x = ori_x;
			y += font->height;

			p++;
			continue;
		}

		u32 codepoint;
		p = utf8_to_codepoint(p, &codepoint);

		struct glyph_set* set = get_glyph_set(font, codepoint);
		stbtt_bakedchar* g = &set->glyphs[codepoint & 0xff];

		f32 w = (f32)(g->x1 - g->x0);
		f32 h = (f32)(g->y1 - g->y0);

		renderer->draw_character(renderer->uptr, set->atlas,
			make_v2f(x + g->xoff, y + g->yoff),
			make_v4f(g->x0, g->y0, w, h),
			colour);

		x += g->xadvance;
	}
}

v2f get_char_dimensions(struct font* font, const char* c) {
	u32 codepoint;
	utf8_to_codepoint(c, &codepoint);

	struct glyph_set* set = get_glyph_set(font, codepoint);
	stbtt_bakedchar* g = &set->glyphs[codepoint & 0xff];
	return make_v2f(g->xadvance, font->height);
}

v2f get_text_dimensions(struct font* font, const char* text) {
	f32 x = 0.0f;

	v2f r = { 0.0f, (f32)font->height };

	const char* p = text;
	while (*p) {
		if (*p == '\n') {
			r.y += font->height;
			x = 0.0f;
			p++;
			continue;
		}

		u32 codepoint;
		p = utf8_to_codepoint(p, &codepoint);

		struct glyph_set* set = get_glyph_set(font, codepoint);
		stbtt_bakedchar* g = &set->glyphs[codepoint & 0xff];

		x += g->xadvance;

		if (x > r.x) { r.x = x; }
	}

	return r;
}

v2f get_text_n_dimensions(struct font* font, const char* text, usize n) {
	f32 x = 0.0f;

	v2f r = { 0.0f, (f32)font->height };

	const char* p = text;
	usize i = 0;
	while (*p && i < n) {
		if (*p == '\n') {
			r.y += font->height;
			x = 0.0f;
			p++;
			i++;
			continue;
		}

		u32 codepoint;
		p = utf8_to_codepoint(p, &codepoint);

		struct glyph_set* set = get_glyph_set(font, codepoint);
		stbtt_bakedchar* g = &set->glyphs[codepoint & 0xff];

		x += g->xadvance;

		if (x > r.x) { r.x = x; }

		i++;
	}

	return r;
}

f32 get_font_height(const struct font* font) {
	return (f32)font->height;
}
