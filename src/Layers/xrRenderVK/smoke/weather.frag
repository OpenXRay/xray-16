#version 450
layout(set = 0, binding = 0) uniform sampler2D albedo_buffer;
layout(set = 0, binding = 1) uniform sampler2D normal_buffer;
layout(set = 0, binding = 2) uniform sampler2D depth_buffer;
layout(set = 1, binding = 0) uniform samplerCube sky_a;
layout(set = 1, binding = 1) uniform samplerCube sky_b;
layout(set = 1, binding = 2) uniform sampler2D clouds_a;
layout(set = 1, binding = 3) uniform sampler2D clouds_b;
layout(push_constant) uniform Weather {
    mat4 inverse_view_projection;
    vec4 direction_ambient;
    vec4 light_color;
    vec4 sky_color;
    vec4 clouds_color;
} weather;
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;

void main()
{
    if (texture(depth_buffer, texcoord).r < 0.99999)
    {
        vec4 albedo = texture(albedo_buffer, texcoord);
        vec3 normal = normalize(texture(normal_buffer, texcoord).rgb * 2.0 - 1.0);
        float diffuse = max(dot(normal, normalize(-weather.direction_ambient.xyz)), 0.0);
        pixel_color = vec4(albedo.rgb * (weather.direction_ambient.w +
            diffuse * weather.light_color.rgb), albedo.a);
        return;
    }

    vec2 clip = texcoord * 2.0 - 1.0;
    vec4 near_point = weather.inverse_view_projection * vec4(clip, 0.0, 1.0);
    vec4 far_point = weather.inverse_view_projection * vec4(clip, 1.0, 1.0);
    vec3 direction = normalize(far_point.xyz / far_point.w - near_point.xyz / near_point.w);
    float angle = weather.sky_color.a;
    direction.xz = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * direction.xz;
    float blend = clamp(weather.light_color.a, 0.0, 1.0);
    vec3 sky = mix(texture(sky_a, direction).rgb, texture(sky_b, direction).rgb, blend) *
        weather.sky_color.rgb;
    vec2 cloud_uv = vec2(atan(direction.z, direction.x) / 6.2831853 + 0.5,
        acos(clamp(direction.y, -1.0, 1.0)) / 3.1415927);
    vec4 cloud = mix(texture(clouds_a, cloud_uv), texture(clouds_b, cloud_uv), blend);
    float alpha = cloud.a * weather.clouds_color.a * smoothstep(-0.05, 0.25, direction.y);
    pixel_color = vec4(mix(sky, cloud.rgb * weather.clouds_color.rgb, alpha), 1.0);
}
