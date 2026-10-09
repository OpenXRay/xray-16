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
    vec4 world = shadow.inverse_view_projection *
        vec4(texcoord.x * 2.0 - 1.0, 1.0 - texcoord.y * 2.0, depth, 1.0);
    if (abs(world.w) < 1e-6) return 1.0;
    vec4 projected = shadow.sun_view_projection * (world / world.w);
    if (projected.w <= 0.0) return 1.0;
    vec3 coord = projected.xyz / projected.w;
    vec2 uv = coord.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) ||
        coord.z <= 0.0 || coord.z >= 1.0) return 1.0;
    // Interpolate comparisons across the four neighbouring shadow texels.
    // Sampling at +/- half a texel with nearest filtering produced 1024px
    // blocks on low sun angles as the camera moved.
    ivec2 size = textureSize(sun_shadow, 0);
    vec2 grid = uv * vec2(size) - 0.5;
    ivec2 base = ivec2(floor(grid));
    vec2 weight = fract(grid);
    float visibility[4];
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
        {
            ivec2 pixel = clamp(base + ivec2(x, y), ivec2(0), size - 1);
            visibility[y * 2 + x] = coord.z - shadow.options.x <=
                texelFetch(sun_shadow, pixel, 0).r ? 1.0 : 0.0;
        }
    return mix(mix(visibility[0], visibility[1], weight.x),
        mix(visibility[2], visibility[3], weight.x), weight.y);
}
void main()
{
    vec4 albedo = texture(albedo_buffer, texcoord);
    vec4 encoded_normal = texture(normal_buffer, texcoord);
    vec3 normal = normalize(encoded_normal.rgb * 2.0 - 1.0);
    float diffuse = max(dot(normal, normalize(-light.direction_ambient.xyz)), 0.0);
    bool static_lightmap = encoded_normal.a < 0.875;
    float hemisphere = static_lightmap ? clamp(encoded_normal.a / 0.75, 0.0, 1.0) : 1.0;
    vec3 sunlight = vec3(0.0);
    if (max(max(light.color.r, light.color.g), light.color.b) > 0.0001)
        sunlight = diffuse * sun_visibility(texture(depth_buffer, texcoord).r) *
            light.color.rgb * (static_lightmap ? albedo.a : 1.0);
    vec3 lit = albedo.rgb * (light.direction_ambient.w + light.grade.w * hemisphere + sunlight);
    lit = (lit - 0.5) * light.grade.z + 0.5;
    lit = pow(max(lit * light.grade.y, 0.0), vec3(1.0 / max(light.grade.x, 0.01)));
    pixel_color = vec4(lit, albedo.a);
}
