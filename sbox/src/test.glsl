#version 440

#begin vertex

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out VSOut {
	vec2 uv;
} vs_out;

layout (std140, set = 1, binding = 0) uniform VertexConfig {
	mat4 transform;
};

void main() {
	vs_out.uv = uv;

	gl_Position = transform * vec4(position, 0.0, 1.0);
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

layout (set = 0, binding = 0) uniform sampler2D image;

layout (std140, set = 2, binding = 0) uniform FragmentConfig {
	vec3 obj_colour;
};

layout (location = 0) in VSOut {
	vec2 uv;
} fs_in;

void main() {
	colour = vec4(obj_colour, 1.0) * texture(image, fs_in.uv);
}

#end fragment
