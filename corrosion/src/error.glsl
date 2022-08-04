#version 440

#begin vertex

layout (location = 0) in vec3 position;

void main() {
	gl_Position = vec4(position, 1.0);
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

void main() {
	colour = vec4(1.0, 0.0, 1.0, 1.0);
}

#end fragment
