#version 440

#begin vertex

layout (location = 0) in vec2 position;
layout (location = 1) in vec4 colour;

layout (binding = 0) uniform Config {
	mat4 camera;
};

layout (location = 0) out VSOut {
	vec4 colour;
} vs_out;

void main() {
	vs_out.colour = colour;

	gl_Position = camera * vec4(position, 0.0, 1.0);
	gl_PointSize = 10.0f;
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec4 colour;
} fs_in;

void main() {
	colour = vec4(fs_in.colour);
}

#end fragment
