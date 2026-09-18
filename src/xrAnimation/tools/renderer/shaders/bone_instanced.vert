#version 450

// Per-vertex data (unit geometry - octahedron or sphere)
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

// Per-instance data (mat4 transform + vec4 color = 5 vec4s)
layout(location = 2) in vec4 in_instance_transform_col0;
layout(location = 3) in vec4 in_instance_transform_col1;
layout(location = 4) in vec4 in_instance_transform_col2;
layout(location = 5) in vec4 in_instance_transform_col3;
layout(location = 6) in vec4 in_instance_color;

// Camera uniform
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view_proj;
} camera;

// Outputs to fragment shader
layout(location = 0) out vec3 out_world_normal;
layout(location = 1) out vec3 out_world_pos;
layout(location = 2) out vec4 out_color;

void main() {
    // Reconstruct instance transform matrix
    mat4 instance_transform = mat4(
        in_instance_transform_col0,
        in_instance_transform_col1,
        in_instance_transform_col2,
        in_instance_transform_col3
    );

    // Transform vertex position
    vec4 world_pos = instance_transform * vec4(in_position, 1.0);
    gl_Position = camera.view_proj * world_pos;

    // Transform normal (use mat3 to ignore translation)
    mat3 normal_matrix = mat3(instance_transform);
    out_world_normal = normal_matrix * in_normal;
    out_world_pos = world_pos.xyz;
    out_color = in_instance_color;
}
