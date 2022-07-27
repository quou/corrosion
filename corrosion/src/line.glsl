#version 440

#begin VERTEX

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 colour;

layout (location = 0) out VSOut {
	vec4 colour;
} vs_out;

layout (std140, binding = 0) uniform VertexUniformData {
	mat4 camera;
};

void main() {
	vs_out.colour = colour;

	gl_Position = camera * vec4(position, 1.0);
}

#end VERTEX

#begin FRAGMENT

layout (location = 0) out vec4 colour;

layout (location = 0) in VSOut {
	vec4 colour;
} fs_in;

void main() {
	colour = fs_in.colour;
}

#end FRAGMENT
