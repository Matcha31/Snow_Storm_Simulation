#version 450 core

in VertexData
{
	vec2 tex_coord;
} in_data;

layout(binding = 0) uniform sampler2D input_tex;

uniform vec2 direction;

layout(location = 0) out vec2 final_color;

void main()
{
	vec2 uv = in_data.tex_coord;

	vec2 value = texture(input_tex, uv).rg * 0.2270270270;
    // weight sum ~= 1
	value += texture(input_tex, uv + direction * 1.3846153846).rg * 0.3162162162;
	value += texture(input_tex, uv - direction * 1.3846153846).rg * 0.3162162162;
	value += texture(input_tex, uv + direction * 3.2307692308).rg * 0.0702702703;
	value += texture(input_tex, uv - direction * 3.2307692308).rg * 0.0702702703;

	final_color = value;
}
