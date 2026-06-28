#version 450 core

in VertexData
{
	vec3 position_ws;
	vec3 normal_ws;
	vec2 tex_coord;
} in_data;

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

struct PhongLight
{
	vec4 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 spot_direction;
	float spot_exponent;
	float spot_cos_cutoff;
	float atten_constant;
	float atten_linear;
	float atten_quadratic;
};

layout(std140, binding = 2) uniform PhongLightsBuffer
{
	vec3 global_ambient_color;
	int lights_count;
	PhongLight lights[8];
};

layout(std140, binding = 3) uniform PhongMaterialBuffer
{
	vec3 ambient;
	vec3 diffuse;
	float alpha;
	vec3 specular;
	float shininess;
} material;

uniform bool has_texture;
uniform float object_snow_strength;
uniform float object_snow_max;
uniform float top_occlusion_bias;

layout(binding = 0) uniform sampler2D material_diffuse_texture;
layout(binding = 1) uniform sampler2D accumulation_texture;
layout(binding = 2) uniform sampler2D sky_depth_texture;

layout(location = 0) out vec4 final_color;

vec3 world_to_sky_position(vec3 position_ws)
{
    // Clip space coord
	vec4 sky_position = sky_camera.projection * sky_camera.view * vec4(position_ws, 1.0);
    // Pespective divide
	sky_position.xyz /= sky_position.w;
    // Convert from [-1, 1] to [0, 1]
	return sky_position.xyz * 0.5 + 0.5;
}

bool uv_inside_texture(vec2 uv)
{
	return uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0;
}

float sample_object_snow(vec3 position_ws)
{
	vec3 sky_position = world_to_sky_position(position_ws);
	vec2 sky_uv = sky_position.xy;

	if (!uv_inside_texture(sky_uv))
	{
		return 0.0;
	}

	float scene_depth = texture(sky_depth_texture, sky_uv).r;

	if (scene_depth < 0.999 && sky_position.z > scene_depth + top_occlusion_bias)
	{
		return 0.0;
	}

	float accumulated_snow = texture(accumulation_texture, sky_uv).g;
	return clamp(accumulated_snow * object_snow_strength, 0.0, object_snow_max);
}

void main()
{
	vec3 N = normalize(in_data.normal_ws);
	vec3 V = normalize(camera.eye_position - in_data.position_ws);

	vec3 amb = global_ambient_color;
	vec3 dif = vec3(0.0);
	vec3 spe = vec3(0.0);

	for (int i = 0; i < lights_count; i++)
	{
		vec3 L_not_normalized = lights[i].position.xyz - in_data.position_ws * lights[i].position.w;
		vec3 L = normalize(L_not_normalized);
		vec3 H = normalize(L + V);

		float Iamb = 1.0;
		float Idif = max(dot(N, L), 0.0);
		float Ispe = (Idif > 0.0) ? pow(max(dot(N, H), 0.0), material.shininess) : 0.0;

		if (lights[i].spot_cos_cutoff != -1.0)
		{
			float spot_factor;
			float spot_cos_angle = dot(-L, lights[i].spot_direction);

			if (spot_cos_angle > lights[i].spot_cos_cutoff)
			{
				spot_factor = pow(spot_cos_angle, lights[i].spot_exponent);
			}
			else
			{
				spot_factor = 0.0;
			}

			Idif *= spot_factor;
			Ispe *= spot_factor;
		}

		if (lights[i].position.w != 0.0)
		{
			float distance_from_light = length(L_not_normalized);
			float atten_factor =
				lights[i].atten_constant +
				lights[i].atten_linear * distance_from_light +
				lights[i].atten_quadratic * distance_from_light * distance_from_light;

			atten_factor = 1.0 / atten_factor;

			Iamb *= atten_factor;
			Idif *= atten_factor;
			Ispe *= atten_factor;
		}

		amb += Iamb * lights[i].ambient;
		dif += Idif * lights[i].diffuse;
		spe += Ispe * lights[i].specular;
	}

	vec3 texture_color = texture(material_diffuse_texture, in_data.tex_coord).rgb;

	vec3 mat_ambient = has_texture ? texture_color : material.ambient;
	vec3 mat_diffuse = has_texture ? texture_color : material.diffuse;
	vec3 mat_specular = material.specular;

    // What changes from lit fragment shader 
    // We add whitening from accumulated snow
	float snow_amount = sample_object_snow(in_data.position_ws);

	mat_ambient = mix(mat_ambient, vec3(1.0), snow_amount);
	mat_diffuse = mix(mat_diffuse, vec3(1.0), snow_amount);
	mat_specular = mix(mat_specular, vec3(0.2), snow_amount);

	vec3 final_light = mat_ambient * amb + mat_diffuse * dif + mat_specular * spe;

	final_color = vec4(final_light, material.alpha);
}
