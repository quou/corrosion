#version 440

#begin vertex

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out VSOut {
	vec2 uv;
} fs_in;

void main() {
	fs_in.uv = uv;

	gl_Position = vec4(position, 0.0, 1.0);
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec2 uv;
} fs_in;

layout (binding = 0) uniform sampler2D src;

void main() {
	vec4 src_col = texture(src, fs_in.uv);

	colour = vec4(src_col.rgb, src_col.a);
}

#end fragment
