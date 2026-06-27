#version 450 core

in ParticleQuadData
{
	vec2 tex_coord;
	flat int hit_terrain;
	flat int hit_object;
} in_data;

layout(binding = 0) uniform sampler2D impact_texture;

uniform float accumulation_strength;

layout(location = 0) out vec2 final_color;

void main()
{
	vec4 texel = texture(impact_texture, in_data.tex_coord);
	float footprint = texel.r * texel.a * accumulation_strength;

	if (footprint < 0.0001)
	{
		discard;
	}

	final_color = vec2(float(in_data.hit_terrain) * footprint, float(in_data.hit_object) * footprint);
}
