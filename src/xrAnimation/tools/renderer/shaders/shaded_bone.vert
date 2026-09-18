#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_color;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view_proj;
} camera;

layout(location = 0) out vec3 out_world_normal;
layout(location = 1) out vec3 out_world_pos;
layout(location = 2) out vec4 out_color;

void main() {
    vec4 world_pos = vec4(in_position, 1.0);
    gl_Position = camera.view_proj * world_pos;

    // Pass through normal and position for lighting in fragment shader
    out_world_normal = in_normal;
    out_world_pos = in_position;
    out_color = in_color;
}
