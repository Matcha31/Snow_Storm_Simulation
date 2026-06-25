<BS#version 450 core

layout(location = 0) out vec4 final_color;

// We only want to know if the fragment is in the cloud or not
void main()
{
	final_color = vec4(1.0);
}
