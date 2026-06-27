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

uniform int display_channel;

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
    vec4 texel = texture(input_tex, in_data.tex_coord);
    float value = max(max(texel.r, texel.g), texel.b);
    if (display_channel == 0) {
        value = texel.r;
    } else if (display_channel == 1) {
        value = texel.g;
    } else if (display_channel == 2) {
        value = texel.b;
    }
    final_color = vec4(vec3(value), 1.0);
}
