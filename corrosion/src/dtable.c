#include <stdio.h>
#include <string.h>

#include "dtable.h"

struct dtable new_dtable(const char* name) {
	struct dtable r = { 0 };

	r.key.len = strlen(name);
	memcpy(r.key.chars, name, cr_min(r.key.len, sizeof r.key.chars));

	r.value.type = dtable_parent;

	return r;
}

struct dtable new_number_dtable(const char* name, f64 value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_number;
	r.value.as.number = value;

	return r;
}

struct dtable new_bool_dtable(const char* name, bool value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_bool;
	r.value.as.boolean = value;

	return r;
}

struct dtable new_string_dtable(const char* name, const char* string) {
	struct dtable r = new_dtable(name);

	usize len = strlen(string);

	r.value.type = dtable_string;

	r.value.as.string = core_alloc(len + 1);
	memcpy(r.value.as.string, string, len);
	r.value.as.string[len] = '\0';

	return r;
}

struct dtable new_string_n_dtable(const char* name, const char* string, usize n) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_string;

	r.value.as.string = core_alloc(n + 1);
	memcpy(r.value.as.string, string, n);
	r.value.as.string[n] = '\0';

	return r;
}

struct dtable new_colour_dtable(const char* name, v4f value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_colour;
	r.value.as.colour = value;

	return r;
}

struct dtable new_v2_dtable(const char* name, v2f value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_v2;
	r.value.as.v2 = value;

	return r;
}

struct dtable new_v3_dtable(const char* name, v3f value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_v3;
	r.value.as.v3 = value;

	return r;
}

struct dtable new_v4_dtable(const char* name, v4f value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_v4;
	r.value.as.v4 = value;

	return r;
}

void dtable_add_child(struct dtable* dt, const struct dtable* child) {
	vector_push(dt->children, *child);
}

static void write_int(void* a, usize size, FILE* file) {
	for (i64 i = (i64)size; i >= 0; i--) {
		u8 byte = *(((u8*)a) + i);
		fprintf(file, "%x", byte);
		printf("%x", byte);
	}
	printf("\n");
}

static void write_dtable_value(const struct dtable_value* val, FILE* file) {
	switch (val->type) {
		case dtable_number:
			fprintf(file, "%g", val->as.number);
			break;
		case dtable_bool:
			fprintf(file, val->as.boolean ? "true" : "false");
			break;
		case dtable_string:
			fprintf(file, "\"%s\"", val->as.string);
			break;
		case dtable_colour: {
			u8 r = (u8)(val->as.colour.x * 255.0f);
			u8 g = (u8)(val->as.colour.y * 255.0f);
			u8 b = (u8)(val->as.colour.z * 255.0f);
			u8 a = (u8)(val->as.colour.w * 255.0f);

			fprintf(file, "colour(#");
			fprintf(file, "%x%x%x", r, g, b);
			if (a != 255) {
				fprintf(file, ", %d", a);
			}
			fprintf(file, ")");

			break;
		}
		case dtable_v2:
			fprintf(file, "v2(%g, %g)", val->as.v2.x, val->as.v2.y);
			break;
		case dtable_v3:
			fprintf(file, "v3(%g, %g, %g)", val->as.v3.x, val->as.v3.y, val->as.v3.z);
			break;
		case dtable_v4:
			fprintf(file, "v4(%g, %g, %g, %g)", val->as.v4.x, val->as.v4.y, val->as.v4.z, val->as.v4.w);
		default: break;
	}

	fwrite(";", 1, 1, file);
}

static void write_indent(u32 count, FILE* file) {
	for (u32 i = 0; i < count; i++) {
		fwrite("\t", 1, 1, file);
	}
}

static void impl_write_dtable(const struct dtable* dt, FILE* file, u32 indent) {
	write_indent(indent, file);

	fwrite(dt->key.chars, 1, dt->key.len, file);
	fwrite(" = ", 1, 3, file);

	if (dt->value.type == dtable_parent) {
		fwrite("{\n", 1, 2, file);

		for (usize i = 0; i < vector_count(dt->children); i++) {
			const struct dtable* ch = dt->children + i;
			impl_write_dtable(ch, file, indent + 1);
		}

		write_indent(indent, file);
		fwrite("}\n", 1, 2, file);
	} else {
		write_dtable_value(&dt->value, file);
		fwrite("\n", 1, 1, file);
	}
}

void write_dtable(const struct dtable* dt, const char* filename) {
	FILE* file = fopen(filename, "w");
	if (!file) {
		error("Failed to fopen `%s' for writing.", filename);
	}

	for (usize i = 0; i < vector_count(dt->children); i++) {
		impl_write_dtable(dt->children + i, file, 0);
	}

	fclose(file);
}

/* Parser. */
enum {
	tok_equal = 0,
	tok_end,
	tok_error,
	tok_key,
	tok_left_brace,
	tok_left_paren,
	tok_number,
	tok_hex,
	tok_right_brace,
	tok_right_paren,
	tok_string,
	tok_semicolon,
	tok_comma,
	tok_false,
	tok_true,
	tok_colour,
	tok_v2,
	tok_v3,
	tok_v4,
	tok_keyword_count
};

struct token {
	u32 type;
	u32 line;
	const char* start;
	usize len;
};

struct parser {
	u32 line;
	const char* cur;
	const char* start;
	const char* source;

	struct token token;
};

static struct token make_token(const struct parser* parser, u32 type) {
	return (struct token) {
		.type = type,
		.line = parser->line,
		.start = parser->start,
		.len = parser->cur - parser->start
	};
}

static struct token error_token(const struct parser* parser, const char* message) {
	return (struct token) {
		.type = tok_error,
		.start = message,
		.len = strlen(message),
		.line = parser->line
	};
}

static char parser_advance(struct parser* parser) {
	parser->cur++;
	return parser->cur[-1];
}

static char parser_peek(const struct parser* parser) {
	return *parser->cur;
}

static char parser_peek_next(const struct parser* parser) {
	if (!*parser->cur) { return '\0'; }
	return parser->cur[1];
}

static bool parser_at_end(const struct parser* parser) {
	return *parser->cur == '\0';
}

static bool parser_match(struct parser* parser, char expected) {
	if (parser_at_end(parser)) { return false; }
	if (*parser->cur != expected) { return false; }
	parser->cur++;
	return false;
}

static void skip_whitespace(struct parser* parser) {
	/* Skip over whitespace and comments */
	while (1) {
		switch (parser_peek(parser)) {
			case ' ':
			case '\r':
			case '\t':
				parser_advance(parser);
				break;
			case '\n':
				parser_advance(parser);
				parser->line++;
				break;
			case '$':
				while (parser_peek(parser) != '\n' && !parser_at_end(parser)) {
					parser_advance(parser);
				}
				break;
			default: return;
		}
	}
}

static bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
	return
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		 c == '_';
}

static bool is_hex(char c) {
	return is_digit(c) ||
		(c >= 'a' && c <= 'f') ||
		(c >= 'A' && c <= 'F');
}

static const char* keywords[] = {
	[tok_colour] = "colour",
	[tok_false]  = "false",
	[tok_true]   = "true",
	[tok_v2]     = "v2",
	[tok_v3]     = "v3",
	[tok_v4]     = "v4"
};

static struct token next_tok(struct parser* parser) {
	skip_whitespace(parser);

	parser->start = parser->cur;

	if (parser_at_end(parser)) { return (struct token) { .type = tok_end }; }

	char c = parser_advance(parser);

	if (is_alpha(c)) {	
		while (is_alpha(parser_peek(parser)) || is_digit(parser_peek(parser))) {
			parser_advance(parser);
		}

		struct token tok = make_token(parser, tok_key);

		for (u32 i = tok_false; i < tok_keyword_count; i++) { /* Check for keywords. */
			if (strlen(keywords[i]) == tok.len && memcmp(parser->start, keywords[i], tok.len) == 0) {
				tok.type = i;
				return tok;
			}
		}

		return tok;
	}

	switch (c) {
		case '=': return make_token(parser, tok_equal);
		case '{': return make_token(parser, tok_left_brace);
		case '(': return make_token(parser, tok_left_paren);
		case '}': return make_token(parser, tok_right_brace);
		case ')': return make_token(parser, tok_right_paren);
		case ';': return make_token(parser, tok_semicolon);
		case ',': return make_token(parser, tok_comma);
		case '-':
			if (is_digit(parser_peek(parser))) {
				parser_advance(parser);
			}

		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9': {
			while (is_digit(parser_peek(parser))) {
				parser_advance(parser);
			}

			if (parser_peek(parser) == '.' && is_digit(parser_peek_next(parser))) {
				parser_advance(parser);

				while (is_digit(parser_peek(parser))) {
					parser_advance(parser);
				}
			}

			return make_token(parser, tok_number);
		} break;
		case '"': {
			parser->start++;
			while (parser_peek(parser) != '"' && !parser_at_end(parser)) {
				if (parser_peek(parser) == '\n') { parser->line++; }
				parser_advance(parser);
			}

			if (parser_at_end(parser)) {
				return error_token(parser, "Unterminated string.");
			}

			struct token tok = make_token(parser, tok_string);
			parser_advance(parser);
			return tok;
		} break;
		case '#': {
			parser->start++;
			while (is_hex(parser_peek(parser))) {
				parser_advance(parser);
			}

			return make_token(parser, tok_hex);
		} break;
	}

	return error_token(parser, "Unexpected character.");
}

static void parse_error(const struct parser* parser, const char* message) {
	error("Parsing DTable on line %u: %s", parser->line, message);
}

/* Returns zero on failure. Recursive-decent parser. */
static bool parse(struct dtable* dt, struct parser* parser) {
	struct token tok = parser->token;

#define advance() tok = next_tok(parser)
#define expect_tok(t_, err_) \
	do { \
		if (tok.type != t_) { \
			parse_error(parser, err_); \
			return false; \
		} \
	} while (0)
#define consume_tok(t_, err_) \
	do { \
		advance(); \
		if (tok.type != t_) { \
			parse_error(parser, err_); \
			return false; \
		} \
	} while (0)
#define after_value() \
	do { \
		consume_tok(tok_semicolon, "Expected `;' after value."); \
	} while (0)

	if (tok.type == tok_end) {
		goto success;
	} else {
		expect_tok(tok_key, "Expected a key.");
	}

	memcpy(dt->key.chars, tok.start, tok.len);
	dt->key.len = tok.len;
	consume_tok(tok_equal, "Expected `=' after key.");
	advance();

	switch (tok.type) {
		case tok_left_brace:
			for (;;) {
				advance();
				parser->token = tok;

				if (tok.type == tok_right_brace) {
					break;
				} else if (parser->token.type == tok_end) {
					parse_error(parser, "Unexpected end.");
					return false;
				}

				dt->value.type = dtable_parent;

				struct dtable child = { 0 };
				if (!parse(&child, parser)) {
					return false;
				}
				dtable_add_child(dt, &child);

				tok = parser->token;
			}
			break;
		case tok_number:
			dt->value.type = dtable_number;
			dt->value.as.number = strtod(tok.start, null);
			after_value();
			break;
		case tok_string:
			dt->value.type = dtable_string;
			dt->value.as.string = core_alloc(tok.len + 1);
			memcpy(dt->value.as.string, tok.start, tok.len);
			dt->value.as.string[tok.len] = '\0';
			after_value();
			break;
		case tok_true:
			dt->value.type = dtable_bool;
			dt->value.as.boolean = true;
			after_value();
			break;
		case tok_false:
			dt->value.type = dtable_bool;
			dt->value.as.boolean = false;
			after_value();
			break;
		case tok_colour: {
			consume_tok(tok_left_paren, "Expected `(' after colour.");
			consume_tok(tok_hex, "Expected a hex value after colour.");

			u32 val = (u32)strtol(tok.start, null, 16);
			u8 a = 255;

			advance();

			if (tok.type == tok_comma) {
				consume_tok(tok_number, "Expected a number after `,'.");
				a = (u8)strtol(tok.start, null, 0);
				consume_tok(tok_right_paren, "Expected `)' after value.");
			} else {
				expect_tok(tok_right_paren, "Expected `)' after value.");
			}

			dt->value.type = dtable_colour;
			dt->value.as.colour = make_rgba(val, a);

			after_value();
		} break;
		case tok_v2: {
			consume_tok(tok_left_paren, "Expected `(' after v2.");
			consume_tok(tok_number, "Expected a number after `('.");

			f64 val1 = strtod(tok.start, null);

			consume_tok(tok_comma, "Expected `,' after number.");
			consume_tok(tok_number, "Expected a number after `,'.");

			f64 val2 = strtod(tok.start, null);

			consume_tok(tok_right_paren, "Expected `)' after number.");

			dt->value.type = dtable_v2;
			dt->value.as.v2 = make_v2f((f32)val1, (f32)val2);

			after_value();
		} break;
		case tok_v3: {
			consume_tok(tok_left_paren, "Expected `(' after v3.");
			consume_tok(tok_number, "Expected a number after `('.");

			f64 val1 = strtod(tok.start, null);

			consume_tok(tok_comma, "Expected `,' after number.");
			consume_tok(tok_number, "Expected a number after `,'.");

			f64 val2 = strtod(tok.start, null);

			consume_tok(tok_comma, "Expected `,' after number.");
			consume_tok(tok_number, "Expected a number after `,'.");

			f64 val3 = strtod(tok.start, null);

			consume_tok(tok_right_paren, "Expected `)' after number.");

			dt->value.type = dtable_v3;
			dt->value.as.v3 = make_v3f((f32)val1, (f32)val2, (f32)val3);

			after_value();
		} break;
		case tok_v4: {
			consume_tok(tok_left_paren, "Expected `(' after v4.");
			consume_tok(tok_number, "Expected a number after `('.");

			f64 val1 = strtod(tok.start, null);

			consume_tok(tok_comma, "Expected `,' after number.");
			consume_tok(tok_number, "Expected a number after `,'.");

			f64 val2 = strtod(tok.start, null);

			consume_tok(tok_comma, "Expected `,' after number.");
			consume_tok(tok_number, "Expected a number after `,'.");

			f64 val3 = strtod(tok.start, null);

			consume_tok(tok_comma, "Expected `,' after number.");
			consume_tok(tok_number, "Expected a number after `,'.");

			f64 val4 = strtod(tok.start, null);

			consume_tok(tok_right_paren, "Expected `)' after number.");

			dt->value.type = dtable_v4;
			dt->value.as.v4 = make_v4f((f32)val1, (f32)val2, (f32)val3, (f32)val4);

			after_value();
		} break;
		case tok_error: {
			parse_error(parser, tok.start);
			return false;
		}
		default:
			parse_error(parser, "Unexpected token.");
			return false;
	}

#undef advance
#undef after_value
#undef consume_tok
#undef expect_tok

success:
	parser->token = tok;

	return true;
}

bool parse_dtable(struct dtable* dt, const char* text) {
	struct parser parser = {
		.line = 1,
		.start  = text,
		.cur    = text,
		.source = text
	};
	
	parser.token.type = tok_error;
	parser.token.start = "a.";

	parser.token = next_tok(&parser);

	while (parser.token.type != tok_end) {
		struct dtable child = { 0 };

		if (!parse(&child, &parser)) {
			return false;
		}

		dtable_add_child(dt, &child);

		parser.token = next_tok(&parser);
	}

	return true;
}

void deinit_dtable(struct dtable* dt) {
	if (dt->value.type == dtable_string) {
		core_free(dt->value.as.string);
	}

	for (usize i = 0; i < vector_count(dt->children); i++) {
		deinit_dtable(dt->children + i);
	}

	free_vector(dt->children);
}

bool dtable_find_child(const struct dtable* dt, const char* key, struct dtable* dst) {
	for (usize i = 0; i < vector_count(dt->children); i++) {
		if (strcmp(dt->children[i].key.chars, key) == 0) {
			*dst = dt->children[i];
			return true;
		}
	}

	return false;
}
