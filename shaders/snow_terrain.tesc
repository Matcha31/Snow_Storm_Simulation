#version 450 core

// Runs 3 times per triangle
layout(vertices = 3) out;

in VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} in_data[];

out VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} out_data[];

// How dense is the subdivision of the triangle
uniform float tessellation_level;

void main()
{
    // Just forward input data
    // Note: gl_InvocationID is 0, 1 or 2 (which vertex we are processing)
	out_data[gl_InvocationID].position_ws = in_data[gl_InvocationID].position_ws;
	out_data[gl_InvocationID].normal_ws = in_data[gl_InvocationID].normal_ws;
	out_data[gl_InvocationID].tex_coord = in_data[gl_InvocationID].tex_coord;

    // Uniform subdivision -> same value
	if (gl_InvocationID == 0)
	{
        // Subdivise traingle outer edges by tessellation level
		gl_TessLevelOuter[0] = tessellation_level;
		gl_TessLevelOuter[1] = tessellation_level;
		gl_TessLevelOuter[2] = tessellation_level;

        // Subdivision inside the triangle
		gl_TessLevelInner[0] = tessellation_level;
	}
}
