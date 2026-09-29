#version 450
layout(set = 0, binding = 0) uniform sampler2D albedo_buffer;
layout(set = 0, binding = 1) uniform sampler2D normal_buffer;
layout(push_constant) uniform Light {
    vec4 direction_ambient;
    vec4 color;
} light;
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;
void main()
{
    vec4 albedo = texture(albedo_buffer, texcoord);
    vec3 normal = normalize(texture(normal_buffer, texcoord).rgb * 2.0 - 1.0);
    float diffuse = max(dot(normal, normalize(-light.direction_ambient.xyz)), 0.0);
    pixel_color = vec4(albedo.rgb * (light.direction_ambient.w +
        diffuse * light.color.rgb), albedo.a);
}
