#version 440

#begin vertex

layout (location = 0) out vec2 uv;

void main() {
	uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(uv * 2.0f + -1.0f, 0.0f, 1.0f);
	uv.y = 1.0 - uv.y;
}

#end vertex

#begin fragment

layout (location = 0) out vec4 colour;

layout (location = 0) in vec2 uv;

layout (binding = 0) uniform sampler2D image;

void main() {
	vec4 tc = texture(image, uv);

	colour = vec4(tc.rgb, tc.a);
}

#end fragment
