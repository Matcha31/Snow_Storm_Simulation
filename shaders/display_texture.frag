#version 450 core

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------
in VertexData
{
	vec2 tex_coord;  // The vertex texture coordinates.
} in_data;

// The texture to display.
layout (binding = 0) uniform sampler2D input_tex;

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
// The final output color.
layout (location = 0) out vec4 final_color;

// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
    // Grayscale only red channel
    float value = texture(input_tex, in_data.tex_coord).r;
    final_color = vec4(vec3(value), 1.0);
    //final_color = texture(input_tex, in_data.tex_coord);
}
