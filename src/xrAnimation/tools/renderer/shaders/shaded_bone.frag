#version 450

layout(location = 0) in vec3 in_world_normal;
layout(location = 1) in vec3 in_world_pos;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main() {
    // Simple directional light from above-right-front
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.5));
    vec3 normal = normalize(in_world_normal);

    // Diffuse lighting with ambient
    float ambient = 0.3;
    float diffuse = max(dot(normal, light_dir), 0.0);
    float light_intensity = ambient + diffuse * 0.7;

    // Apply lighting to color
    vec3 lit_color = in_color.rgb * light_intensity;
    out_color = vec4(lit_color, in_color.a);
}
