#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view_proj;
} camera;

layout(location = 0) out vec4 out_color;

void main() {
    gl_Position = camera.view_proj * vec4(in_position, 1.0);
    out_color = in_color;
}
