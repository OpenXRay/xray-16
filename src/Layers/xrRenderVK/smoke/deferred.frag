#version 450
layout(set = 0, binding = 0) uniform sampler2D albedo_buffer;
layout(set = 0, binding = 1) uniform sampler2D normal_buffer;
layout(set = 0, binding = 2) uniform sampler2D depth_buffer;
layout(set = 0, binding = 3) uniform sampler2D sun_shadow;
layout(set = 0, binding = 4, std140) uniform ShadowConstants {
    mat4 inverse_view_projection;
    mat4 sun_view_projection;
    vec4 options;
    vec4 ambient_color;
    vec4 hemi_color;
} shadow;
layout(set = 0, binding = 5) uniform sampler2D surface_buffer;
layout(push_constant) uniform Light {
    vec4 direction_ambient;
    vec4 color;
    vec4 grade;
} light;
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;
float sun_visibility(float depth, vec2 screen_uv)
{
    vec4 world = shadow.inverse_view_projection *
        vec4(screen_uv.x * 2.0 - 1.0, 1.0 - screen_uv.y * 2.0, depth, 1.0);
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
    ivec2 center = ivec2(floor(grid + 0.5));
    vec2 fraction = grid - vec2(center);
    float lit = 0.0, total = 0.0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
        {
            vec2 weight = max(vec2(1.5) - abs(vec2(x, y) - fraction), vec2(0.0));
            float w = weight.x * weight.y;
            ivec2 pixel = clamp(center + ivec2(x, y), ivec2(0), size - 1);
            float visibility = coord.z - shadow.options.x <= texelFetch(sun_shadow, pixel, 0).r ? 1.0 : 0.0;
            lit += visibility * w;
            total += w;
        }
    return lit / max(total, 1e-6);
}
void main()
{
    bool diagnostic = shadow.options.y > 0.5;
    bool four_panes = diagnostic && shadow.options.y < 1.5;
    vec2 pane = four_panes ? floor(min(texcoord * 2.0, vec2(1.9999))) : vec2(0.0);
    int panel = four_panes ? int(pane.x) + 2 * int(pane.y) :
        diagnostic ? clamp(int(shadow.options.y) - 2, 0, 3) : 0;
    vec2 screen_uv = four_panes ? fract(texcoord * 2.0) : texcoord;
    float depth = texture(depth_buffer, screen_uv).r;
    if (diagnostic && depth >= 0.99999 && panel != 0)
    {
        pixel_color = vec4(0.04, 0.08, 0.18, 1.0);
        return;
    }
    vec4 albedo = texture(albedo_buffer, screen_uv);
    vec4 encoded_normal = texture(normal_buffer, screen_uv);
    vec4 surface = texture(surface_buffer, screen_uv);
    vec3 normal = normalize(encoded_normal.rgb * 2.0 - 1.0);
    float diffuse = max(dot(normal, normalize(-light.direction_ambient.xyz)), 0.0);
    bool static_lightmap = encoded_normal.a < 0.8;
    float hemisphere = static_lightmap ? clamp(encoded_normal.a / 0.75, 0.0, 1.0) :
        clamp((encoded_normal.a - 0.875) * 8.0, 0.0, 1.0);
    if (diagnostic && panel == 1)
    {
        pixel_color = vec4(vec3(sun_visibility(depth, screen_uv)), 1.0);
        return;
    }
    if (diagnostic && panel == 2)
    {
        pixel_color = vec4(static_lightmap ? albedo.a : 1.0,
            hemisphere, static_lightmap ? 0.0 : 1.0, 1.0);
        return;
    }
    if (diagnostic && panel == 3)
    {
        pixel_color = vec4(albedo.rgb, 1.0);
        return;
    }
    float sun_scale = 0.0;
    vec3 sunlight = vec3(0.0);
    if (max(max(light.color.r, light.color.g), light.color.b) > 0.0001)
    {
        sun_scale = sun_visibility(depth, screen_uv);
        sunlight = diffuse * sun_scale * light.color.rgb;
    }
    vec3 lit = albedo.rgb * (shadow.ambient_color.rgb + shadow.hemi_color.rgb * hemisphere + sunlight);
    if (surface.r > 0.0 && sun_scale > 0.0)
    {
        vec4 world_clip = shadow.inverse_view_projection *
            vec4(screen_uv.x * 2.0 - 1.0, 1.0 - screen_uv.y * 2.0,
                depth, 1.0);
        vec4 near_clip = shadow.inverse_view_projection *
            vec4(screen_uv.x * 2.0 - 1.0, 1.0 - screen_uv.y * 2.0, 0.0, 1.0);
        vec3 view_ray = normalize(near_clip.xyz / near_clip.w - world_clip.xyz / world_clip.w);
        vec3 half_dir = normalize(view_ray - normalize(light.direction_ambient.xyz));
        float specular = pow(max(dot(normal, half_dir), 0.0), mix(4.0, 64.0, surface.r));
        lit += specular * surface.r * mix(0.04, 0.2, surface.g) *
            light.color.rgb * sun_scale;
    }
    lit += albedo.rgb * surface.b;
    lit = (lit - 0.5) * light.grade.z + 0.5;
    lit = pow(max(lit * light.grade.y, 0.0), vec3(1.0 / max(light.grade.x, 0.01)));
    pixel_color = vec4(lit, albedo.a);
}
