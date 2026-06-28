#version 450 core

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

uniform float tessellation_level;

void main()
{
	out_data[gl_InvocationID].position_ws = in_data[gl_InvocationID].position_ws;
	out_data[gl_InvocationID].normal_ws = in_data[gl_InvocationID].normal_ws;
	out_data[gl_InvocationID].tex_coord = in_data[gl_InvocationID].tex_coord;

	if (gl_InvocationID == 0)
	{
		gl_TessLevelOuter[0] = tessellation_level;
		gl_TessLevelOuter[1] = tessellation_level;
		gl_TessLevelOuter[2] = tessellation_level;
		gl_TessLevelInner[0] = tessellation_level;
	}
}
