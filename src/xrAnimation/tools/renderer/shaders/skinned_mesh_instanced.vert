#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_bone_weights;
layout(location = 4) in uvec4 in_bone_indices;

layout(location = 5) in vec4 in_instance_transform_col0;
layout(location = 6) in vec4 in_instance_transform_col1;
layout(location = 7) in vec4 in_instance_transform_col2;
layout(location = 8) in vec4 in_instance_transform_col3;

layout(location = 9) in uint in_bone_matrix_offset;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view_proj;
} camera;

// Model-space bone transforms (updated per-frame)
layout(std430, set = 0, binding = 1) readonly buffer ModelMatrices {
    mat4 matrices[];
} model_matrices;

// Inverse bind pose matrices (uploaded once, shared across all instances)
layout(std430, set = 0, binding = 2) readonly buffer InverseBindPoses {
    mat4 matrices[];
} inverse_bind_poses;

// Joint remap buffer (palette index → skeleton joint index)
layout(std430, set = 0, binding = 3) readonly buffer JointRemaps {
    uint indices[];
} joint_remaps;

layout(push_constant) uniform SkinningOptions {
    uint flags;
} skinning_options;

const uint FLAG_GPU_SKINNING = 1u;

layout(location = 0) out vec3 out_world_normal;
layout(location = 1) out vec2 out_uv;

// Skinning matrix computation. When GPU skinning is enabled we upload skeleton-order
// model matrices and multiply inverse bind pose on the GPU. When disabled we upload
// palette-order skinning matrices that already include the inverse bind pose.
mat4 accumulate_skinning(uint base_index) {
    mat4 skin_matrix = mat4(0.0);
    bool gpu_skinning = (skinning_options.flags & FLAG_GPU_SKINNING) != 0u;

    if (in_bone_weights.x > 0.0) {
        uint palette_idx = in_bone_indices.x;
        mat4 skinning_mat;
        if (gpu_skinning) {
            uint joint_idx = joint_remaps.indices[palette_idx];
            skinning_mat = model_matrices.matrices[base_index + joint_idx]
                         * inverse_bind_poses.matrices[palette_idx];
        } else {
            skinning_mat = model_matrices.matrices[base_index + palette_idx];
        }
        skin_matrix += skinning_mat * in_bone_weights.x;
    }
    if (in_bone_weights.y > 0.0) {
        uint palette_idx = in_bone_indices.y;
        mat4 skinning_mat;
        if (gpu_skinning) {
            uint joint_idx = joint_remaps.indices[palette_idx];
            skinning_mat = model_matrices.matrices[base_index + joint_idx]
                         * inverse_bind_poses.matrices[palette_idx];
        } else {
            skinning_mat = model_matrices.matrices[base_index + palette_idx];
        }
        skin_matrix += skinning_mat * in_bone_weights.y;
    }
    if (in_bone_weights.z > 0.0) {
        uint palette_idx = in_bone_indices.z;
        mat4 skinning_mat;
        if (gpu_skinning) {
            uint joint_idx = joint_remaps.indices[palette_idx];
            skinning_mat = model_matrices.matrices[base_index + joint_idx]
                         * inverse_bind_poses.matrices[palette_idx];
        } else {
            skinning_mat = model_matrices.matrices[base_index + palette_idx];
        }
        skin_matrix += skinning_mat * in_bone_weights.z;
    }
    if (in_bone_weights.w > 0.0) {
        uint palette_idx = in_bone_indices.w;
        mat4 skinning_mat;
        if (gpu_skinning) {
            uint joint_idx = joint_remaps.indices[palette_idx];
            skinning_mat = model_matrices.matrices[base_index + joint_idx]
                         * inverse_bind_poses.matrices[palette_idx];
        } else {
            skinning_mat = model_matrices.matrices[base_index + palette_idx];
        }
        skin_matrix += skinning_mat * in_bone_weights.w;
    }

    float weight_sum = in_bone_weights.x + in_bone_weights.y + in_bone_weights.z + in_bone_weights.w;
    if (weight_sum <= 0.0) {
        // Fallback to identity to avoid zeroed transforms on degenerate weights.
        skin_matrix = mat4(1.0);
    }

    return skin_matrix;
}

void main() {
    mat4 instance_transform = mat4(
        in_instance_transform_col0,
        in_instance_transform_col1,
        in_instance_transform_col2,
        in_instance_transform_col3
    );

    uint base_index = in_bone_matrix_offset;

    // TEMP DEBUG: Verify we can read the buffers
    // For first vertex of first instance, palette 0 should map to skeleton joint 2
    // Test: Read joint_remaps[0] - should be 2
    // Test: Read model_matrices[2] - should be valid
    // Test: Read inverse_bind_poses[0] - should be non-identity

    mat4 skin_matrix = accumulate_skinning(base_index);

    vec4 local_position = skin_matrix * vec4(in_position, 1.0);
    vec3 local_normal = (mat3(skin_matrix) * in_normal);

    vec4 world_position = instance_transform * local_position;
    gl_Position = camera.view_proj * world_position;

    mat3 normal_matrix = mat3(instance_transform);
    out_world_normal = normalize(normal_matrix * local_normal);
    out_uv = in_uv;
}
