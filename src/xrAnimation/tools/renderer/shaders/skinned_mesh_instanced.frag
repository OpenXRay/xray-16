#version 450

layout(location = 0) in vec3 in_world_normal;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 normal = normalize(in_world_normal);
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3));

    float diffuse_term = max(dot(normal, light_dir), 0.0);
    vec3 ambient = vec3(0.2);
    vec3 diffuse = vec3(0.8) * diffuse_term;

    out_color = vec4(ambient + diffuse, 1.0);
}
