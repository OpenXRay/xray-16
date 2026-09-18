#version 450

// Per-vertex data (bone line endpoints)
layout(location = 0) in vec3 in_position;

// Per-instance data (4 vec4s = mat4)
layout(location = 1) in vec4 in_instance_transform_col0;
layout(location = 2) in vec4 in_instance_transform_col1;
layout(location = 3) in vec4 in_instance_transform_col2;
layout(location = 4) in vec4 in_instance_transform_col3;

// Uniforms - Camera view-projection matrix
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view_proj;
} camera;

void main() {
    // Reconstruct instance transform matrix from per-instance columns
    mat4 instance_transform = mat4(
        in_instance_transform_col0,
        in_instance_transform_col1,
        in_instance_transform_col2,
        in_instance_transform_col3
    );

    // Transform vertex: ViewProj * InstanceTransform * Position
    gl_Position = camera.view_proj * instance_transform * vec4(in_position, 1.0);
}
