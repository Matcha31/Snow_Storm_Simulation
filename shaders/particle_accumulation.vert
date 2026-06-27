#version 450 core

struct Particle
{
	vec4 position;
	vec4 velocity_delay;
	ivec4 flags;
};

layout(std430, binding = 4) buffer ParticleBuffer
{
	Particle particles[];
};

out ParticlePointData
{
	vec3 position_ws;
	flat int hit_terrain;
	flat int hit_object;
	flat uint particle_id;
} out_data;

void main()
{
	Particle particle = particles[gl_VertexID];

	out_data.position_ws = particle.position.xyz;
	out_data.hit_terrain = particle.flags.x;
	out_data.hit_object = particle.flags.y;
	out_data.particle_id = uint(gl_VertexID);

	gl_Position = vec4(particle.position.xyz, 1.0);
}
