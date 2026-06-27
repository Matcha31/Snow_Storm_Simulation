#version 450 core

struct Particle {
	vec4 position;
	vec4 velocity_delay;
	ivec4 flags;
};

layout(std430, binding = 4) buffer ParticleBuffer {
	Particle particles[];
};

out ParticlePointData {
	vec3 position_ws;
	float delay;
    flat int hit_terrain;
    flat int hit_object;
	flat int released;
} out_data;

void main() {
    // We run once per particle
	Particle particle = particles[gl_VertexID]; // index of current particle being drawn

	out_data.position_ws = particle.position.xyz;
	out_data.delay = particle.velocity_delay.w;
    out_data.hit_terrain = particle.flags.x;
    out_data.hit_object = particle.flags.y;
	out_data.released = particle.flags.z;

	gl_Position = vec4(particle.position.xyz, 1.0);
}
