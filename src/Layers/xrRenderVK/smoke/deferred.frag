#version 450
layout(set = 0, binding = 0) uniform sampler2D albedo_buffer;
layout(set = 0, binding = 1) uniform sampler2D normal_buffer;
layout(set = 0, binding = 2) uniform sampler2D depth_buffer;
layout(set = 0, binding = 3) uniform sampler2D sun_shadow;
layout(set = 0, binding = 4, std140) uniform ShadowConstants {
    mat4 inverse_view_projection;
    mat4 sun_view_projection;
    vec4 options;
} shadow;
layout(push_constant) uniform Light {
    vec4 direction_ambient;
    vec4 color;
    vec4 grade;
} light;
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;
float sun_visibility(float depth)
{
    vec4 world = shadow.inverse_view_projection * vec4(texcoord * 2.0 - 1.0, depth, 1.0);
    if (abs(world.w) < 1e-6) return 1.0;
    vec4 projected = shadow.sun_view_projection * (world / world.w);
    if (projected.w <= 0.0) return 1.0;
    vec3 coord = projected.xyz / projected.w;
    vec2 uv = coord.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) ||
        coord.z <= 0.0 || coord.z >= 1.0) return 1.0;
    vec2 texel = 1.0 / vec2(textureSize(sun_shadow, 0));
    float lit = 0.0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            lit += coord.z - shadow.options.x <= texture(sun_shadow, uv + vec2(x, y) * texel).r ? 1.0 : 0.0;
    return lit / 9.0;
}
void main()
{
    vec4 albedo = texture(albedo_buffer, texcoord);
    vec3 normal = normalize(texture(normal_buffer, texcoord).rgb * 2.0 - 1.0);
    float diffuse = max(dot(normal, normalize(-light.direction_ambient.xyz)), 0.0);
    vec3 lit = albedo.rgb * (light.direction_ambient.w +
        diffuse * sun_visibility(texture(depth_buffer, texcoord).r) * light.color.rgb);
    lit = (lit - 0.5) * light.grade.z + 0.5;
    lit = pow(max(lit * light.grade.y, 0.0), vec3(1.0 / max(light.grade.x, 0.01)));
    pixel_color = vec4(mix(lit, vec3(dot(lit, vec3(0.299, 0.587, 0.114))), light.grade.w), albedo.a);
}
