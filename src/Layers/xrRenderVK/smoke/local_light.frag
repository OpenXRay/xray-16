#version 450
layout(set = 0, binding = 0) uniform sampler2D albedo_buffer;
layout(set = 0, binding = 1) uniform sampler2D normal_buffer;
layout(set = 0, binding = 2) uniform sampler2D depth_buffer;
layout(set = 1, binding = 0) uniform sampler2DArray shadow_array;
layout(set = 1, binding = 1, std140) uniform LocalLight {
    mat4 inverse_view_projection;
    mat4 shadow_matrices[6];
    vec4 position_range;
    vec4 direction_cone;
    vec4 color_type;
    vec4 shadow_params;
} light;
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;

float visibility(vec3 world, vec3 delta)
{
    if (light.shadow_params.z < 0.5) return 1.0;
    int face = 0;
    if (light.color_type.w < 0.5)
    {
        vec3 a = abs(delta);
        face = a.x >= a.y && a.x >= a.z ? (delta.x >= 0.0 ? 0 : 1) :
            a.y >= a.z ? (delta.y >= 0.0 ? 2 : 3) : (delta.z >= 0.0 ? 4 : 5);
    }
    vec4 clip = light.shadow_matrices[face] * vec4(world, 1.0);
    if (clip.w <= 0.0) return 1.0;
    vec3 projected = clip.xyz / clip.w;
    vec2 uv = projected.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) ||
        projected.z <= 0.0 || projected.z >= 1.0) return 1.0;
    float layer = light.shadow_params.x + float(face);
    vec2 step_uv = 1.0 / vec2(textureSize(shadow_array, 0).xy);
    float lit = 0.0;
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            lit += projected.z - light.shadow_params.y <=
                texture(shadow_array, vec3(uv + (vec2(x, y) - 0.5) * step_uv, layer)).r ? 1.0 : 0.0;
    return lit * 0.25;
}

void main()
{
    float depth = texture(depth_buffer, texcoord).r;
    if (depth >= 0.99999) { pixel_color = vec4(0.0); return; }
    vec4 world_clip = light.inverse_view_projection *
        vec4(texcoord.x * 2.0 - 1.0, 1.0 - texcoord.y * 2.0, depth, 1.0);
    vec3 world = world_clip.xyz / max(abs(world_clip.w), 1e-6) * sign(world_clip.w);
    vec3 delta = world - light.position_range.xyz;
    float distance = length(delta);
    float range = max(light.position_range.w, 0.01);
    if (distance >= range || distance <= 1e-5) { pixel_color = vec4(0.0); return; }
    vec3 normal = normalize(texture(normal_buffer, texcoord).rgb * 2.0 - 1.0);
    float diffuse = max(dot(normal, -delta / distance), 0.0);
    float normalized_distance = distance / range;
    float attenuation = max(1.0 - normalized_distance * normalized_distance, 0.0);
    if (light.color_type.w >= 0.5)
        attenuation *= smoothstep(light.direction_cone.w, min(1.0, light.direction_cone.w + 0.05),
            dot(normalize(delta), normalize(light.direction_cone.xyz)));
    vec3 color = texture(albedo_buffer, texcoord).rgb * light.color_type.rgb *
        diffuse * attenuation * visibility(world, delta);
    pixel_color = vec4(color, 0.0);
}
