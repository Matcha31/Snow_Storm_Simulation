#version 450 core

in VertexData
{
	vec2 tex_coord;
} in_data;

layout(binding = 0) uniform sampler2D input_tex;

uniform vec2 direction; // vertical or horizontal
uniform int blur_radius; // how far we sample
uniform float blur_sigma; // how much influence

layout(location = 0) out vec2 final_color;

// Center : higher weight, farther : lower weight
float gaussian_weight(float x, float sigma)
{
	return exp(-(x * x) / (2.0 * sigma * sigma));
}

void main()
{
	vec2 sum = vec2(0.0);
	float weight_sum = 0.0;

	for (int i = -32; i <= 32; i++) // fixed loop more GPU friendly
	{
		if (abs(i) > blur_radius) // out of range
        {
			continue;
        }

		float weight = gaussian_weight(float(i), blur_sigma);
		vec2 uv = in_data.tex_coord + direction * float(i); // neighboor sample coord

		sum += texture(input_tex, uv).rg * weight;
		weight_sum += weight;
	}

    // Normalize to avoid change in snow intensity
	final_color = sum / weight_sum;
}
