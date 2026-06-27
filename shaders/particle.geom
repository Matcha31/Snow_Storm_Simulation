#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(std140, binding = 0) uniform CameraBuffer {
	mat4 projection;
	mat4 projection_inv;
	mat4 view;
	mat4 view_inv;
	mat3 view_it;
	vec3 eye_position;
} camera;

in ParticlePointData {
	vec3 position_ws;
	float delay;
    flat int hit_terrain;
    flat int hit_object;
	flat int released;
} in_data[];

out ParticleQuadData {
	vec2 tex_coord;
} out_data;

uniform float particle_size;

void emit_particle_vertex(vec3 center, vec2 offset, vec2 tex_coord) {
    // If camera rotates we need to rotate the snowflake too
	vec3 camera_right = normalize(vec3(camera.view_inv[0]));
	vec3 camera_up = normalize(vec3(camera.view_inv[1]));
	vec3 position_ws = center + (camera_right * offset.x + camera_up * offset.y) * particle_size;

	out_data.tex_coord = tex_coord;
	gl_Position = camera.projection * camera.view * vec4(position_ws, 1.0);
	EmitVertex();
}

// Transform each particle into a textured quad
void main() {
    // If the particle is unreleased or hit terrain/object : don't draw
    if (in_data[0].released == 0 || in_data[0].hit_terrain == 1 || in_data[0].hit_object == 1) {
        return;
    }

    // Point
	vec3 center = in_data[0].position_ws;

    // Quad : billboard with 4 vertices 
    // OpenGL will create 2 triangles 
	emit_particle_vertex(center, vec2(-0.5, -0.5), vec2(0.0, 0.0));
	emit_particle_vertex(center, vec2( 0.5, -0.5), vec2(1.0, 0.0));
	emit_particle_vertex(center, vec2(-0.5,  0.5), vec2(0.0, 1.0));
	emit_particle_vertex(center, vec2( 0.5,  0.5), vec2(1.0, 1.0));

	EndPrimitive();
}
