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
layout(set = 1, binding = 0) uniform samplerCube sky_a;
layout(set = 1, binding = 1) uniform samplerCube sky_b;
layout(set = 1, binding = 2) uniform sampler2D clouds_a;
layout(set = 1, binding = 3) uniform sampler2D clouds_b;
layout(set = 1, binding = 4) uniform samplerCube environment_a;
layout(set = 1, binding = 5) uniform samplerCube environment_b;
layout(push_constant) uniform Weather {
    vec4 ray_base;
    vec4 ray_dx;
    vec4 ray_dy;
    vec4 direction_ambient;
    vec4 light_color;
    vec4 grade;
    vec4 sky_color;
    vec4 clouds_color;
} weather;
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

// R2's hmodel samples the weather environment in the world-normal direction,
// then squares the tinted sample for diffuse indirect light. Its material
// table uses h^0.75, h^0.9 and h for diffuse slices 0, 1 and 2/3.
// Reproduce those slices analytically instead of baking a generic hemi tint.
float material_diffuse(float hemi, float material)
{
    float h = clamp(hemi, 0.0, 1.0);
    float category = clamp(material * 3.0, 0.0, 3.0);
    float oren = pow(h, 0.75);
    float blinn = pow(h, 0.90);
    return category < 1.0 ? mix(oren, blinn, category) :
        category < 2.0 ? mix(blinn, h, category - 1.0) : h;
}

vec3 environment_indirect(vec3 normal, float hemi, float material)
{
    float blend = clamp(weather.light_color.a, 0.0, 1.0);
    vec3 environment = mix(textureLod(environment_a, normal, 0.0).rgb,
        textureLod(environment_b, normal, 0.0).rgb, blend);
    // CoP's env_color is 2*hemi_color and the R2 combine pass doubles it.
    vec3 tinted = environment * shadow.hemi_color.rgb * 4.0;
    return tinted * tinted * material_diffuse(hemi, material);
}

// The second channel of R2's 3D material table controls the reflected
// environment. Reproduce its four material slices without another texture
// allocation or a new descriptor on the mobile combine path.
float material_specular(float hemi, float reflection, float material)
{
    float h = clamp(hemi, 0.0, 1.0);
    float ls = clamp(reflection, 0.0, 1.0) * pow(h, 1.0 / 32.0);
    float oren = 0.5 * pow(ls, 16.0);
    float blinn = pow(ls, 24.0);
    float phong = pow(min(ls * 1.01, 1.0), 128.0);
    float metal = max(max(abs(1.0 - abs(0.05 * sin(33.0 * h) + h - ls)),
        abs(1.0 - abs(0.05 * cos(33.0 * h * ls) + h - ls))),
        abs(1.0 - abs(h - ls)));
    metal = pow(clamp(metal, 0.0, 1.0), 24.0) * pow(h, 1.0 / 7.0);
    float category = clamp(material * 3.0, 0.0, 3.0);
    return category < 1.0 ? mix(oren, blinn, category) :
        category < 2.0 ? mix(blinn, phong, category - 1.0) :
        mix(phong, metal, category - 2.0);
}

void main()
{
    // The four-pane view repeats one G-buffer; modes 2..5 fill the screen
    // with the corresponding panel at its native resolution.
    bool diagnostic = shadow.options.y > 0.5;
    bool comparison = diagnostic && shadow.options.y > 5.5;
    bool four_panes = diagnostic && (shadow.options.y < 1.5 ||
        (comparison && shadow.options.y < 6.5));
    vec2 pane = four_panes ? floor(min(texcoord * 2.0, vec2(1.9999))) : vec2(0.0);
    int panel = four_panes ? int(pane.x) + 2 * int(pane.y) :
        diagnostic && !comparison ? clamp(int(shadow.options.y) - 2, 0, 3) : 0;
    // Comparison panels: normal, full hemisphere, full sun visibility,
    // both overrides. Mode 7 is the previous flat-hemi renderer; 8/9
    // isolate the hemisphere/sun overrides at full resolution.
    int comparison_mode = comparison ? (four_panes ? panel :
        int(shadow.options.y) == 7 ? 4 : int(shadow.options.y) - 7) : 0;
    vec2 screen_uv = four_panes ? fract(texcoord * 2.0) : texcoord;
    if (texture(depth_buffer, screen_uv).r < 0.99999)
    {
        float depth = texture(depth_buffer, screen_uv).r;
        vec4 albedo = texture(albedo_buffer, screen_uv);
        vec4 encoded_normal = texture(normal_buffer, screen_uv);
        vec4 surface = texture(surface_buffer, screen_uv);
        vec3 normal = normalize(encoded_normal.rgb * 2.0 - 1.0);
        float diffuse = max(dot(normal, normalize(-weather.direction_ambient.xyz)), 0.0);
        bool static_lightmap = encoded_normal.a < 0.8;
        float hemisphere = static_lightmap ? clamp(encoded_normal.a / 0.75, 0.0, 1.0) :
            clamp((encoded_normal.a - 0.875) * 8.0, 0.0, 1.0);
        float sun_scale = 0.0;
        vec3 sunlight = vec3(0.0);
        // Most rainy frames have no sun. Avoid nine depth fetches per pixel
        // when the result would be multiplied by black anyway.
        if (diagnostic && !comparison && panel == 1)
        {
            float visibility = sun_visibility(depth, screen_uv);
            pixel_color = vec4(vec3(visibility), 1.0);
            return;
        }
        if (diagnostic && !comparison && panel == 2)
        {
            // R: baked static-sun channel (unused by dynamic sun), G: hemisphere,
            // B: blue denotes an object without a static lightmap.
            pixel_color = vec4(static_lightmap ? albedo.a : 1.0,
                hemisphere, static_lightmap ? 0.0 : 1.0, 1.0);
            return;
        }
        if (diagnostic && !comparison && panel == 3)
        {
            pixel_color = vec4(albedo.rgb, 1.0);
            return;
        }
        if (max(max(weather.light_color.r, weather.light_color.g), weather.light_color.b) > 0.0001)
        {
            sun_scale = (comparison_mode == 2 || comparison_mode == 3) ? 1.0 :
                sun_visibility(depth, screen_uv);
            sunlight = diffuse * sun_scale * weather.light_color.rgb;
        }
        float tested_hemi = (comparison_mode == 1 || comparison_mode == 3) ? 1.0 : hemisphere;
        vec3 indirect = comparison_mode == 4 ?
            shadow.hemi_color.rgb * tested_hemi :
            environment_indirect(normal, tested_hemi, surface.g);
        vec3 lit = albedo.rgb * (shadow.ambient_color.rgb + indirect + sunlight);
        if (surface.r > 0.0 && comparison_mode != 4)
        {
            vec4 world_clip = shadow.inverse_view_projection *
                vec4(screen_uv.x * 2.0 - 1.0, 1.0 - screen_uv.y * 2.0, depth, 1.0);
            vec3 world_position = world_clip.xyz / world_clip.w;
            vec4 camera_clip = shadow.inverse_view_projection * vec4(0.0, 0.0, 0.0, 1.0);
            vec3 incident = normalize(world_position - camera_clip.xyz / camera_clip.w);
            vec3 reflection = reflect(incident, normal);
            float angle = 0.5 + 0.5 * dot(reflection, incident);
            reflection.y = reflection.y * 2.0 - 1.0;
            float blend = clamp(weather.light_color.a, 0.0, 1.0);
            vec3 environment = mix(textureLod(environment_a, reflection, 0.0).rgb,
                textureLod(environment_b, reflection, 0.0).rgb, blend);
            vec3 tinted = environment * shadow.hemi_color.rgb * 4.0;
            lit += tinted * tinted * material_specular(tested_hemi, angle, surface.g) *
                surface.r;
        }
        if (surface.r > 0.0 && sun_scale > 0.0)
        {
            vec4 world_clip = shadow.inverse_view_projection *
                vec4(screen_uv.x * 2.0 - 1.0, 1.0 - screen_uv.y * 2.0, depth, 1.0);
            vec4 near_clip = shadow.inverse_view_projection *
                vec4(screen_uv.x * 2.0 - 1.0, 1.0 - screen_uv.y * 2.0, 0.0, 1.0);
            vec3 view_ray = normalize(near_clip.xyz / near_clip.w - world_clip.xyz / world_clip.w);
            vec3 half_dir = normalize(view_ray - normalize(weather.direction_ambient.xyz));
            float specular = pow(max(dot(normal, half_dir), 0.0),
                mix(4.0, 64.0, surface.r));
            lit += specular * surface.r * mix(0.04, 0.2, surface.g) *
                weather.light_color.rgb * sun_scale;
        }
        lit += albedo.rgb * surface.b;
        vec2 fog_rg = unpackHalf2x16(floatBitsToUint(weather.ray_base.w));
        vec2 fog_b_near = unpackHalf2x16(floatBitsToUint(weather.ray_dx.w));
        vec2 fog_far_projection = unpackHalf2x16(floatBitsToUint(weather.ray_dy.w));
        const float camera_near = 0.2;
        float projection_far = max(fog_far_projection.y, camera_near + 1.0);
        float view_distance = camera_near * projection_far /
            max(projection_far - depth * (projection_far - camera_near), 0.01);
        vec3 ray = weather.ray_base.xyz + screen_uv.x * weather.ray_dx.xyz +
            screen_uv.y * weather.ray_dy.xyz;
        view_distance *= length(ray) / projection_far;
        float fog = clamp((view_distance - fog_b_near.y) /
            max(fog_far_projection.x - fog_b_near.y, 0.01), 0.0, 1.0);
        lit = mix(lit, vec3(fog_rg, fog_b_near.x), fog);
        lit = (lit - 0.5) * weather.grade.z + 0.5;
        lit = pow(max(lit * weather.grade.y, 0.0), vec3(1.0 / max(weather.grade.x, 0.01)));
        pixel_color = vec4(lit, albedo.a);
        return;
    }

    if (diagnostic && !comparison && panel != 0)
    {
        pixel_color = vec4(0.04, 0.08, 0.18, 1.0);
        return;
    }
    vec3 direction = normalize(weather.ray_base.xyz + screen_uv.x * weather.ray_dx.xyz +
        screen_uv.y * weather.ray_dy.xyz);
    float angle = weather.sky_color.a;
    direction.xz = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * direction.xz;
    float blend = clamp(weather.light_color.a, 0.0, 1.0);
    vec3 sky = mix(texture(sky_a, direction).rgb, texture(sky_b, direction).rgb, blend) *
        weather.sky_color.rgb;
    // CoP projects both cloud layers from the XZ plane of a flattened
    // hemisphere. Longitude/latitude coordinates introduce a polar pinwheel
    // and a vertical wrap seam, especially in fully overcast weather.
    vec2 cloud_params = unpackHalf2x16(floatBitsToUint(weather.grade.w));
    float rotation = cloud_params.y;
    vec3 dome = normalize(vec3(direction.x, direction.y * 25.0, direction.z));
    vec2 planar = mat2(cos(rotation), -sin(rotation),
        sin(rotation), cos(rotation)) * dome.xz;
    float clock = cloud_params.x * 0.1;
    vec2 uv0 = planar * 0.7 + vec2(0.70710678, 0.70710678) * clock * 0.1;
    vec2 uv1 = planar * 2.8 + vec2(0.92387953, 0.38268343) * clock * 0.05;
    vec4 cloud_a = texture(clouds_a, uv0) + texture(clouds_a, uv1);
    vec4 cloud_b = texture(clouds_b, uv0) + texture(clouds_b, uv1);
    vec4 cloud = mix(cloud_a, cloud_b, blend);
    float alpha = clamp(cloud.a * weather.clouds_color.a *
        pow(max(dome.y, 0.0), 25.0), 0.0, 1.0);
    vec3 lit = mix(sky, cloud.rgb * weather.clouds_color.rgb, alpha);
    lit = (lit - 0.5) * weather.grade.z + 0.5;
    lit = pow(max(lit * weather.grade.y, 0.0), vec3(1.0 / max(weather.grade.x, 0.01)));
    pixel_color = vec4(lit, 1.0);
}
