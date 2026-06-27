#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;
	mat4 projection_inv;
	mat4 view;
	mat4 view_inv;
	mat3 view_it;
	vec3 eye_position;
} camera;

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

in ParticlePointData
{
	vec3 position_ws;
	flat int hit_terrain;
	flat int hit_object;
	flat uint particle_id;
} in_data[];

out ParticleQuadData
{
	vec2 tex_coord;
	flat int hit_terrain;
	flat int hit_object;
} out_data;

uniform float accumulation_particle_size;
uniform int frame_index;
uniform float max_delay;

uint hash_uint(uint x)
{
	x ^= x >> 16;
	x *= 2246822519u;
	x ^= x >> 13;
	x *= 3266489917u;
	x ^= x >> 16;
	return x;
}

float random01(uint seed)
{
	return float(hash_uint(seed)) / 4294967295.0;
}

void emit_particle_vertex(vec3 center, vec2 offset, vec2 tex_coord)
{
	vec3 camera_right = normalize(vec3(camera.view_inv[0]));
	vec3 camera_up = normalize(vec3(camera.view_inv[1]));
	vec3 position_ws = center + (camera_right * offset.x + camera_up * offset.y) * accumulation_particle_size;

	out_data.tex_coord = tex_coord;
	out_data.hit_terrain = in_data[0].hit_terrain;
	out_data.hit_object = in_data[0].hit_object;

	gl_Position = camera.projection * camera.view * vec4(position_ws, 1.0);
	EmitVertex();
}

void reset_particle(uint particle_id)
{
	uint seed = particle_id * 747796405u + uint(frame_index) * 2891336453u + 91u;

	particles[particle_id].flags = ivec4(0, 0, 0, 0);
	particles[particle_id].velocity_delay = vec4(0.0, 0.0, 0.0, random01(seed) * max_delay);
}

void main()
{
	if (in_data[0].hit_terrain == 0 && in_data[0].hit_object == 0)
	{
		return;
	}

	vec3 center = in_data[0].position_ws;

	emit_particle_vertex(center, vec2(-0.5, -0.5), vec2(0.0, 0.0));
	emit_particle_vertex(center, vec2( 0.5, -0.5), vec2(1.0, 0.0));
	emit_particle_vertex(center, vec2(-0.5,  0.5), vec2(0.0, 1.0));
	emit_particle_vertex(center, vec2( 0.5,  0.5), vec2(1.0, 1.0));

	EndPrimitive();

	reset_particle(in_data[0].particle_id);
}
