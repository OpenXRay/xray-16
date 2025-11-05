#version 450

// Output color
layout(location = 0) out vec4 out_color;

void main() {
    // Simple white color for skeleton bones
    out_color = vec4(1.0, 1.0, 1.0, 1.0);
}
