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

layout(std140, binding = 5) uniform SkyCameraBuffer
{
	mat4 projection;
	mat4 projection_inv;
	mat4 view;
	mat4 view_inv;
	mat3 view_it;
	vec3 eye_position;
} sky_camera;

// Original triangle
in VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} in_data[];

// New tesselated vertex
out VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} out_data;

layout(binding = 1) uniform sampler2D accumulation_texture;
layout(binding = 2) uniform sampler2D snow_height_texture;

uniform float debug_displacement; // Snow height for debug
uniform float snow_height_scale;
uniform float max_snow_height;
uniform float terrain_edge_width;
uniform bool use_accumulation_displacement;
uniform float height_texture_tiling; 
uniform float height_texture_strength; // How much we vary

vec3 world_to_sky_position(vec3 position_ws)
{
	vec4 sky_position = sky_camera.projection * sky_camera.view * vec4(position_ws, 1.0);
	sky_position.xyz /= sky_position.w;
	return sky_position.xyz * 0.5 + 0.5;
}

bool uv_inside_texture(vec2 uv)
{
	return uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0;
}

// Check if the vertex is on the edge of the terrain
bool is_terrain_edge(vec2 tex_coord)
{
	return tex_coord.x <= terrain_edge_width ||
		   tex_coord.x >= 1.0 - terrain_edge_width ||
		   tex_coord.y <= terrain_edge_width ||
		   tex_coord.y >= 1.0 - terrain_edge_width;
}

// Get accumulated snow amount wit accumulation texture
float sample_accumulated_height(vec3 position_ws, vec2 tex_coord)
{
	vec3 sky_position = world_to_sky_position(position_ws);
	vec2 sky_uv = sky_position.xy;

	if (!uv_inside_texture(sky_uv)) {
		return 0.0;
	}

	float accumulated_snow = texture(accumulation_texture, sky_uv).r;
	float height = accumulated_snow * snow_height_scale;

    // Vary height
    float height_sample = texture(snow_height_texture, tex_coord * height_texture_tiling).r;
	float height_variation = mix(1.0, 0.75 + height_sample * 0.5, height_texture_strength);

	height *= height_variation;

	return clamp(height, 0.0, max_snow_height);
}

void main()
{
    // Barycentric coordinates of original triangle
	vec3 barycentric = gl_TessCoord;

    // Get new tesselated vertex data
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

    // Compute height
	float height = debug_displacement;

	if (use_accumulation_displacement) {
		height += sample_accumulated_height(position_ws, tex_coord);
	}
	if (is_terrain_edge(tex_coord)) {
		height = 0.0;
	}

	position_ws.y += height;

	out_data.position_ws = position_ws;
	out_data.normal_ws = normal_ws;
	out_data.tex_coord = tex_coord;

    // Place the new vertex on the screen
	gl_Position = camera.projection * camera.view * vec4(position_ws, 1.0);
}
