#version 450 core

layout(triangles, equal_spacing, ccw) in;

layout(std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;
	mat4 projection_inv;
	mat4 view;
	mat4 view_inv;
	mat3 view_it;
	vec3 eye_position;
} camera;

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
} out_data;

uniform float debug_displacement;

void main()
{
	vec3 barycentric = gl_TessCoord;

	vec3 position_ws =
		barycentric.x * in_data[0].position_ws +
		barycentric.y * in_data[1].position_ws +
		barycentric.z * in_data[2].position_ws;

	vec3 normal_ws = normalize(
		barycentric.x * in_data[0].normal_ws +
		barycentric.y * in_data[1].normal_ws +
		barycentric.z * in_data[2].normal_ws
	);

	vec2 tex_coord =
		barycentric.x * in_data[0].tex_coord +
		barycentric.y * in_data[1].tex_coord +
		barycentric.z * in_data[2].tex_coord;

	position_ws.y += debug_displacement;

	out_data.position_ws = position_ws;
	out_data.normal_ws = normal_ws;
	out_data.tex_coord = tex_coord;

	gl_Position = camera.projection * camera.view * vec4(position_ws, 1.0);
}
