#version 440

#begin vertex

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 colour;
layout (location = 3) in float use_texture;

layout (location = 0) out VSOut {
	vec2 uv;
	vec4 colour;
	float use_texture;
} vs_out;

layout (std140, binding = 0) uniform VertexUniformData {
	mat4 projection;
};

void main() {
	vs_out.uv = uv;
	vs_out.colour = colour;
	vs_out.use_texture = use_texture;

	gl_Position = projection * vec4(position, 0.0, 1.0);
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec2 uv;
	vec4 colour;
	float use_texture;
} fs_in;

layout (binding = 1) uniform sampler2D atlas;

void main() {
	vec4 texture_colour = vec4(1.0);

	if (fs_in.use_texture > 0.0f) {
		texture_colour = texture(atlas, fs_in.uv);
	}

	colour = texture_colour * fs_in.colour;
}

#end fragment
