#version 450
layout(set = 0, binding = 0) uniform sampler2D albedo_buffer;
layout(set = 0, binding = 1) uniform sampler2D normal_buffer;
layout(push_constant) uniform Light {
    vec4 direction_ambient;
    vec4 color;
    vec4 grade;
} light;
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;
void main()
{
    vec4 albedo = texture(albedo_buffer, texcoord);
    vec3 normal = normalize(texture(normal_buffer, texcoord).rgb * 2.0 - 1.0);
    float diffuse = max(dot(normal, normalize(-light.direction_ambient.xyz)), 0.0);
    vec3 lit = albedo.rgb * (light.direction_ambient.w + diffuse * light.color.rgb);
    lit = (lit - 0.5) * light.grade.z + 0.5;
    lit = pow(max(lit * light.grade.y, 0.0), vec3(1.0 / max(light.grade.x, 0.01)));
    pixel_color = vec4(mix(lit, vec3(dot(lit, vec3(0.299, 0.587, 0.114))), light.grade.w), albedo.a);
}
