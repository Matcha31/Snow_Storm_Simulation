#version 450 core

in ParticleQuadData {
	vec2 tex_coord;
} in_data;

layout(binding = 0) uniform sampler2D particle_texture;

uniform float particle_alpha;

layout(location = 0) out vec4 final_color;

void main() {
	vec4 texel = texture(particle_texture, in_data.tex_coord);

    // Make black part transparent
    // Red channel is used as intenity mask
	float alpha = texel.r * texel.a * particle_alpha;

	if (alpha < 0.01)
	{
		discard;
	}

	final_color = vec4(vec3(1.0), alpha);
}
