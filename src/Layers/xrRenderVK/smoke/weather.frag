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
layout(set = 1, binding = 0) uniform samplerCube sky_a;
layout(set = 1, binding = 1) uniform samplerCube sky_b;
layout(set = 1, binding = 2) uniform sampler2D clouds_a;
layout(set = 1, binding = 3) uniform sampler2D clouds_b;
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
    if (texture(depth_buffer, texcoord).r < 0.99999)
    {
        float depth = texture(depth_buffer, texcoord).r;
        vec4 albedo = texture(albedo_buffer, texcoord);
        vec3 normal = normalize(texture(normal_buffer, texcoord).rgb * 2.0 - 1.0);
        float diffuse = max(dot(normal, normalize(-weather.direction_ambient.xyz)), 0.0);
        vec3 lit = albedo.rgb * (weather.direction_ambient.w +
            diffuse * sun_visibility(depth) * weather.light_color.rgb);
        vec2 fog_rg = unpackHalf2x16(floatBitsToUint(weather.ray_base.w));
        vec2 fog_b_near = unpackHalf2x16(floatBitsToUint(weather.ray_dx.w));
        vec2 fog_far_projection = unpackHalf2x16(floatBitsToUint(weather.ray_dy.w));
        const float camera_near = 0.2;
        float projection_far = max(fog_far_projection.y, camera_near + 1.0);
        float view_distance = camera_near * projection_far /
            max(projection_far - depth * (projection_far - camera_near), 0.01);
        vec3 ray = weather.ray_base.xyz + texcoord.x * weather.ray_dx.xyz +
            texcoord.y * weather.ray_dy.xyz;
        view_distance *= length(ray) / projection_far;
        float fog = clamp((view_distance - fog_b_near.y) /
            max(fog_far_projection.x - fog_b_near.y, 0.01), 0.0, 1.0);
        lit = mix(lit, vec3(fog_rg, fog_b_near.x), fog);
        lit = (lit - 0.5) * weather.grade.z + 0.5;
        lit = pow(max(lit * weather.grade.y, 0.0), vec3(1.0 / max(weather.grade.x, 0.01)));
        pixel_color = vec4(mix(lit, vec3(dot(lit, vec3(0.299, 0.587, 0.114))), weather.grade.w), albedo.a);
        return;
    }

    vec3 direction = normalize(weather.ray_base.xyz + texcoord.x * weather.ray_dx.xyz +
        texcoord.y * weather.ray_dy.xyz);
    float angle = weather.sky_color.a;
    direction.xz = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * direction.xz;
    float blend = clamp(weather.light_color.a, 0.0, 1.0);
    vec3 sky = mix(texture(sky_a, direction).rgb, texture(sky_b, direction).rgb, blend) *
        weather.sky_color.rgb;
    vec2 cloud_uv = vec2(atan(direction.z, direction.x) / 6.2831853 + 0.5,
        acos(clamp(direction.y, -1.0, 1.0)) / 3.1415927);
    vec4 cloud = mix(texture(clouds_a, cloud_uv), texture(clouds_b, cloud_uv), blend);
    float alpha = cloud.a * weather.clouds_color.a * smoothstep(-0.05, 0.25, direction.y);
    vec3 lit = mix(sky, cloud.rgb * weather.clouds_color.rgb, alpha);
    lit = (lit - 0.5) * weather.grade.z + 0.5;
    lit = pow(max(lit * weather.grade.y, 0.0), vec3(1.0 / max(weather.grade.x, 0.01)));
    pixel_color = vec4(mix(lit, vec3(dot(lit, vec3(0.299, 0.587, 0.114))), weather.grade.w), 1.0);
}
