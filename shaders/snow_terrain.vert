#version 450 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;

layout(std140, binding = 1) uniform ModelData
{
	mat4 model;
	mat4 model_inv;
	mat3 model_it;
};

out VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} out_data;

// Same as object.vert without gl_Position
// No longer the final stage that places the vertex on the screen
// Prepares original triangle vertices
void main()
{
	out_data.position_ws = vec3(model * position);
	out_data.normal_ws = normalize(model_it * normal);
	out_data.tex_coord = tex_coord;
}
