#version 450 core

layout(location = 0) out vec4 final_color;

uniform float surface_value;

// To classify the fragment, we use the surface value :
// 0.0 : background, 0.5 : terrain, 1.0 : object
void main() {
	final_color = vec4(vec3(surface_value), 1.0);
}
