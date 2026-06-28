#version 450 core

// Lit frag but with normal map

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
uniform float snow_height_scale;
uniform float max_snow_height;
uniform float terrain_edge_width;
uniform bool use_accumulation_displacement;
uniform float height_texture_tiling;
uniform float height_texture_strength;
uniform float terrain_world_size;
uniform float sky_world_size;
uniform bool use_snow_normal_map;
uniform float snow_normal_tiling;
uniform float snow_normal_strength;

layout(binding = 0) uniform sampler2D material_diffuse_texture;
layout(binding = 1) uniform sampler2D accumulation_texture;
layout(binding = 2) uniform sampler2D snow_height_texture;
layout(binding = 3) uniform sampler2D snow_normal_texture;

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

bool is_terrain_edge(vec2 tex_coord)
{
    return tex_coord.x <= terrain_edge_width ||
        tex_coord.x >= 1.0 - terrain_edge_width ||
        tex_coord.y <= terrain_edge_width ||
        tex_coord.y >= 1.0 - terrain_edge_width;
}

float sample_height_from_uv(vec2 sky_uv, vec2 tex_coord)
{
    if (!use_accumulation_displacement) {
        return 0.0;
    }

    if (!uv_inside_texture(sky_uv)) {
        return 0.0;
    }

    if (is_terrain_edge(tex_coord)) {
        return 0.0;
    }

    float accumulated_snow = texture(accumulation_texture, sky_uv).r;
    float height = accumulated_snow * snow_height_scale;

    float height_sample = texture(snow_height_texture, tex_coord * height_texture_tiling).r;
    float height_variation = mix(1.0, 0.75 + height_sample * 0.5, height_texture_strength);

    height *= height_variation;

    return clamp(height, 0.0, max_snow_height);
}

// Compute tangent, bitangent and normal
void compute_snow_frame(out vec3 tangent, out vec3 bitangent, out vec3 normal)
{
    vec3 sky_position = world_to_sky_position(in_data.position_ws);
    vec2 sky_uv = sky_position.xy;

    // Sample distances
    float sky_texel = 1.0 / float(textureSize(accumulation_texture, 0).x);
    float world_step = sky_texel * sky_world_size;
    float terrain_texel = world_step / terrain_world_size;

    // Sample neighbooring heights (central differencing)
    float r = sample_height_from_uv(sky_uv + vec2(sky_texel, 0.0), in_data.tex_coord + vec2(terrain_texel, 0.0));
    float l = sample_height_from_uv(sky_uv - vec2(sky_texel, 0.0), in_data.tex_coord - vec2(terrain_texel, 0.0));
    float t = sample_height_from_uv(sky_uv + vec2(0.0, sky_texel), in_data.tex_coord + vec2(0.0, terrain_texel));
    float b = sample_height_from_uv(sky_uv - vec2(0.0, sky_texel), in_data.tex_coord - vec2(0.0, terrain_texel));

    tangent = normalize(vec3(2.0 * world_step, r - l, 0.0));
    bitangent = normalize(vec3(0.0, t - b, 2.0 * world_step));
    normal = normalize(cross(bitangent, tangent));

    // Point upward
    if (normal.y < 0.0)
    {
        normal = -normal;
        tangent = -tangent;
    }
}

vec3 apply_snow_normal_map(vec3 tangent, vec3 bitangent, vec3 normal)
{
    if (!use_snow_normal_map)
    {
        return normal;
    }

    // Convert from [0, 1] to [-1, 1] : stored as colors
    vec3 normal_sample = texture(snow_normal_texture, in_data.tex_coord * snow_normal_tiling).rgb;
    normal_sample = normal_sample * 2.0 - 1.0;

    normal_sample.xy *= snow_normal_strength;
    normal_sample.z = max(normal_sample.z, 0.05); // Avoid flat
    normal_sample = normalize(normal_sample); // Avoid pointing down

    // Transform into world space using frame
    vec3 mapped_normal = normalize(
            tangent * normal_sample.x +
            bitangent * normal_sample.y +
            normal * normal_sample.z
            );

    if (mapped_normal.y < 0.0)
    {
        mapped_normal = normal; // Avoid downward
    }

    return mapped_normal;
}

void main()
{
    // Computes the tangent frame.
    // What changes from lit fragment shader
    vec3 tangent;
    vec3 bitangent;
    vec3 macro_normal;
    compute_snow_frame(tangent, bitangent, macro_normal);
    vec3 N = apply_snow_normal_map(tangent, bitangent, macro_normal);

    vec3 V = normalize(camera.eye_position - in_data.position_ws);

    // Sets the starting coefficients.
    vec3 amb = global_ambient_color;
    vec3 dif = vec3(0.0);
    vec3 spe = vec3(0.0);

    // Processes all the lights.
    for (int i = 0; i < lights_count; i++)
    {
        vec3 L_not_normalized = lights[i].position.xyz - in_data.position_ws * lights[i].position.w;
        vec3 L = normalize(L_not_normalized);
        vec3 H = normalize(L + V);

        // Calculates the basic Phong factors.
        float Iamb = 1.0;
        float Idif = max(dot(N, L), 0.0);
        float Ispe = (Idif > 0.0) ? pow(max(dot(N, H), 0.0), material.shininess) : 0.0;

        // Calculates spot light factor.
        if (lights[i].spot_cos_cutoff != -1.0)
        {
            float spot_factor;
            float spot_cos_angle = dot(-L, lights[i].spot_direction);
            if (spot_cos_angle > lights[i].spot_cos_cutoff)
            {
                spot_factor = pow(spot_cos_angle, lights[i].spot_exponent);
            }
            else spot_factor = 0.0;

            Iamb *= 1.0;
            Idif *= spot_factor;
            Ispe *= spot_factor;
        }

        // Calculates attenuation point/spot lights.
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

        // Applies the factors to light color.
        amb += Iamb * lights[i].ambient;
        dif += Idif * lights[i].diffuse;
        spe += Ispe * lights[i].specular;
    }

    // Computes the material - we either use material or texture as ambient and diffuse parts.
    vec3 mat_ambient = has_texture ? texture(material_diffuse_texture, in_data.tex_coord).rgb :  material.ambient;
    vec3 mat_diffuse = has_texture ? texture(material_diffuse_texture, in_data.tex_coord).rgb :  material.diffuse;
    vec3 mat_specular = material.specular;

    // Computes the final light color.
    vec3 final_light = mat_ambient * amb + mat_diffuse * dif + material.specular * spe;

    // Outputs the final light color.
    final_color = vec4(final_light, material.alpha);
}
