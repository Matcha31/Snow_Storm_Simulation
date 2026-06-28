#version 450 core

layout(location = 0) out vec4 final_color;

// Fragment is in the cloud -> white
void main() {
	final_color = vec4(1.0);
}
