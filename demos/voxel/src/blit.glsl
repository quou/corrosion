#version 440

#begin vertex

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out VSOut {
	vec2 uv;
} vs_out;

void main() {
	vs_out.uv = uv;

	gl_Position = vec4(position, 0.0, 1.0);
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec2 uv;
} fs_in;

layout (binding = 0) uniform sampler2D image;
layout (binding = 1) uniform sampler2D depth_tex;

layout (binding = 2) uniform Config {
	vec2 image_size;
};

void main() {
	float gamma = 2.2;
	colour = vec4(pow(texture(image, fs_in.uv).rgb, vec3(1.0 / gamma)), 1.0);
}

#end fragment
