#version 440

#begin vertex

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 colour;
layout (location = 3) in float use_texture;
layout (location = 4) in float radius;
layout (location = 5) in vec4 rect;

layout (location = 0) out VSOut {
	vec2 uv;
	vec4 colour;
	float use_texture;
	float radius;
	vec4 rect;
	vec2 frag_pos;
} fs_in;

layout (std140, binding = 0) uniform VertexUniformData {
	mat4 projection;
};

void main() {
	fs_in.uv = uv;
	fs_in.colour = colour;
	fs_in.use_texture = use_texture;
	fs_in.radius = radius;
	fs_in.rect = rect;

	vec4 p = (projection * vec4(position, 0.0, 1.0));

	fs_in.frag_pos = position;

	gl_Position = p;
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec2 uv;
	vec4 colour;
	float use_texture;
	float radius;
	vec4 rect;
	vec2 frag_pos;
} fs_in;

layout (binding = 1) uniform sampler2D atlas;

float rounded_box_sdf(vec2 pos, vec2 size, float r) {
	return length(max(abs(pos) - size + r, 0.0)) - r;
}

void main() {
	vec4 texture_colour = vec4(1.0);

	vec2 position   = fs_in.rect.xy;
	vec2 dimentions = fs_in.rect.zw;

	float softness = 0.5;

	float r = fs_in.radius;

	float d = rounded_box_sdf(fs_in.frag_pos - position - (dimentions * 0.5), dimentions * 0.5, r);
	float a = 1.0 - smoothstep(0.0, softness * 2.0, d);

	if (fs_in.use_texture > 0.0f) {
		texture_colour = texture(atlas, fs_in.uv);
	}

	colour = mix(vec4(0.0), texture_colour * fs_in.colour, a);
}

#end fragment
