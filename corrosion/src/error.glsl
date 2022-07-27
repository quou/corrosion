#version 440

#begin VERTEX

layout (location = 0) in vec3 position;

void main() {
	gl_Position = vec4(position, 1.0);
}

#end VERTEX

#begin FRAGMENT

layout (location = 0) out vec4 colour;

void main() {
	colour = vec4(1.0, 0.0, 1.0, 1.0);
}

#end FRAGMENT
