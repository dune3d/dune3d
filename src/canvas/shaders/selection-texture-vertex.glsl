#version 330 core

in vec2 position;
out vec2 pos_to_frag;

void main() {
	gl_Position = vec4(position, -1, 1);
    pos_to_frag = position;
}
