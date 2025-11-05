# Vulkan Renderer Design Document
## ozz_animation_viewer - High-Performance Instanced Rendering

### Overview

A lightweight, modern Vulkan renderer for `ozz_animation_viewer` that replaces ozz-animation's sample framework with a high-performance, instanced rendering pipeline.

### Goals

- ✅ **Remove ozz sample dependencies** - Fully independent viewer
- ✅ **GPU instancing** - Single draw call per object type
- ✅ **GPU skinning** - Compute bone transforms in shaders
- ✅ **Multi-instance support** - Efficiently render 100+ animated characters
- ✅ **Debug visualization** - IK targets, skeleton bones, collision shapes
- ✅ **Cross-platform** - Windows, Linux (WSL2 compatible)

### Architecture

```
src/xrAnimation/tools/renderer/
├── VulkanRenderer.h/cpp              # Main renderer class
├── VulkanDevice.h/cpp                # Device, queue, swapchain management
├── VulkanBuffer.h/cpp                # Buffer abstraction (vertex, index, uniform, storage)
├── VulkanPipeline.h/cpp              # Pipeline state objects
├── VulkanDescriptors.h/cpp           # Descriptor sets and layouts
├── VulkanCommandBuffer.h/cpp         # Command buffer recording
├── InstancedSkeletonRenderer.h/cpp   # Skeleton line rendering
├── InstancedMeshRenderer.h/cpp       # Skinned mesh rendering
├── DebugRenderer.h/cpp               # Debug lines, points, axes
├── Camera.h/cpp                      # Arcball camera controller
└── shaders/
    ├── skeleton_instanced.vert       # Skeleton vertex shader
    ├── skeleton_instanced.frag       # Skeleton fragment shader
    ├── skinned_mesh_instanced.vert   # Skinned mesh vertex shader
    ├── skinned_mesh_instanced.frag   # Skinned mesh fragment shader
    ├── debug_lines.vert              # Debug line vertex shader
    └── debug_lines.frag              # Debug line fragment shader
```

---

## 🔀 Parallel Work Plan

This implementation is designed to maximize developer productivity through parallel development. Below are the explicit parallelization opportunities for each phase:

### Phase 1 Parallelization (4 Parallel Tracks)

**TRACK 1A: VulkanDevice** (Core infrastructure)
- Files: `VulkanDevice.h/cpp`
- Dependencies: Vulkan SDK, GLFW
- Blocking: All other tracks depend on device being initialized
- Priority: **CRITICAL - Start first**

**TRACK 1B: VulkanBuffer** (Independent of device details)
- Files: `VulkanBuffer.h/cpp`
- Dependencies: VMA header only (no runtime dependency on Track 1A)
- Can be developed in parallel with 1A
- Priority: **HIGH**

**TRACK 1C: VulkanCommandBuffer** (Independent API design)
- Files: `VulkanCommandBuffer.h/cpp`
- Dependencies: Conceptual only
- Can be developed in parallel with 1A/1B
- Priority: **HIGH**

**TRACK 1D: CMake Shader Compilation** (Build system)
- Files: `CMakeLists.txt`, shader compilation functions
- Dependencies: None (just need Vulkan SDK path)
- Can be developed completely independently
- Priority: **MEDIUM**

**Integration Point**: All tracks converge when implementing `VulkanPipeline` and `VulkanDescriptors`

### Phase 2 Parallelization (3 Parallel Tracks)

**TRACK 2A: Skeleton Shaders** (GLSL development)
- Files: `shaders/skeleton_instanced.vert`, `shaders/skeleton_instanced.frag`
- Dependencies: None (pure GLSL)
- Can start immediately after shader compilation setup
- Priority: **HIGH**

**TRACK 2B: Camera Controller** (Math/Input)
- Files: `Camera.h/cpp`
- Dependencies: GLFW, ozz math (no Vulkan required)
- Completely independent track
- Priority: **MEDIUM**

**TRACK 2C: InstancedSkeletonRenderer** (Graphics pipeline)
- Files: `InstancedSkeletonRenderer.h/cpp`
- Dependencies: Tracks 1A/1B/1C complete, Track 2A in progress
- Can start skeleton data generation while shaders being written
- Priority: **HIGH**

**Integration Point**: All tracks converge for skeleton rendering test

### Phase 3 Parallelization (3 Parallel Tracks)

**TRACK 3A: Skinned Mesh Shaders** (GLSL development)
- Files: `shaders/skinned_mesh_instanced.vert`, `shaders/skinned_mesh_instanced.frag`
- Dependencies: None (pure GLSL)
- Can start immediately after Phase 2 completes
- Priority: **HIGH**

**TRACK 3B: Mesh Data Conversion** (Data pipeline)
- Files: Mesh conversion utilities, vertex format definitions
- Dependencies: ozz::sample::Mesh structure knowledge
- Independent of Track 3A/3C implementation
- Priority: **MEDIUM**

**TRACK 3C: InstancedMeshRenderer** (Graphics pipeline)
- Files: `InstancedMeshRenderer.h/cpp`
- Dependencies: Tracks 1A/1B/1C complete, Track 3A in progress
- Can design API while shaders being written
- Priority: **HIGH**

**Integration Point**: All tracks converge for skinned mesh rendering test

### Phase 4 Parallelization (2 Parallel Tracks)

**TRACK 4A: Debug Shaders** (GLSL development)
- Files: `shaders/debug_lines.vert`, `shaders/debug_lines.frag`
- Dependencies: None (pure GLSL)
- Can start in parallel with Phase 3
- Priority: **LOW** (can defer)

**TRACK 4B: DebugRenderer** (Immediate-mode API)
- Files: `DebugRenderer.h/cpp`
- Dependencies: Phase 1 complete
- Can start in parallel with Phase 3
- Priority: **LOW** (can defer)

**Note**: Phase 4 can run concurrently with Phase 5 integration work

### Phase 5 Parallelization (Sequential)

- Integration work is mostly sequential
- Can parallelize: Testing while implementing ImGui integration
- Final polish items can be distributed

### Recommended Development Order

**Week 1:**
```
Day 1-2: [1A + 1B + 1C + 1D] in parallel
Day 3-4: VulkanDescriptors + VulkanPipeline (sequential, needs 1A/1B/1C)
Day 5:   Simple triangle test (validation)
```

**Week 2:**
```
Day 1-2: [2A + 2B] in parallel, start 2C when 2A has basic structure
Day 3-4: Complete 2C, integrate all tracks
Day 5:   Test 100 static skeletons
```

**Week 3:**
```
Day 1-2: [3A + 3B] in parallel, start 3C when 3A has basic structure
Day 3-4: Complete 3C, integrate all tracks
Day 5:   Test 100 animated characters
```

**Week 4:**
```
Day 1-2: [4A + 4B] in parallel
Day 3-5: Debug visualization complete, start Phase 5
```

**Week 5:**
```
Day 1-3: Integration work
Day 4-5: Testing, performance profiling, polish
```

---

## Phase 1: Vulkan Foundation (Week 1)

### 1.1 Vulkan Instance & Device Setup

**File**: `VulkanDevice.h/cpp`

- [ ] Create Vulkan instance with validation layers (debug builds)
- [ ] Enumerate physical devices, select best GPU
- [ ] Create logical device with graphics + present queues
- [ ] Set up swapchain (triple buffering, FIFO/Mailbox present mode)
- [ ] Create render pass (color attachment + optional depth)
- [ ] Create framebuffers for swapchain images
- [ ] Set up synchronization primitives (semaphores, fences)

**Dependencies**:
- Vulkan SDK (system package or bundled)
- GLFW for window/surface creation
- VMA (Vulkan Memory Allocator) for efficient memory management

**Key Classes**:
```cpp
class VulkanDevice {
public:
    void Initialize(GLFWwindow* window);
    void Shutdown();

    VkDevice GetDevice() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkQueue GetGraphicsQueue() const;
    VkQueue GetPresentQueue() const;
    VkSwapchainKHR GetSwapchain() const;

    VkImage GetCurrentSwapchainImage() const;
    VkFramebuffer GetCurrentFramebuffer() const;

    void BeginFrame();  // Acquire next image
    void EndFrame();    // Present

private:
    VkInstance instance_;
    VkPhysicalDevice physical_device_;
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
    VkSwapchainKHR swapchain_;
    VkRenderPass render_pass_;
    xr_vector<VkFramebuffer> framebuffers_;

    // Synchronization
    xr_vector<VkSemaphore> image_available_semaphores_;
    xr_vector<VkSemaphore> render_finished_semaphores_;
    xr_vector<VkFence> in_flight_fences_;
    uint32_t current_frame_ = 0;
};
```

---

### 1.2 Buffer Abstraction

**File**: `VulkanBuffer.h/cpp`

- [ ] Implement VulkanBuffer wrapper class
- [ ] Support vertex buffers (position, normal, uv, weights, indices)
- [ ] Support uniform buffers (per-frame, per-instance)
- [ ] Support storage buffers (bone matrices for GPU skinning)
- [ ] Implement staging buffer for CPU→GPU transfers
- [ ] Use VMA for automatic memory management

**Key Class**:
```cpp
class VulkanBuffer {
public:
    void Create(VkDeviceSize size, VkBufferUsageFlags usage,
                VmaMemoryUsage memory_usage);
    void Destroy();

    void* Map();
    void Unmap();
    void Upload(const void* data, VkDeviceSize size);

    VkBuffer GetBuffer() const { return buffer_; }
    VkDeviceSize GetSize() const { return size_; }

private:
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkDeviceSize size_ = 0;
    void* mapped_data_ = nullptr;
};
```

---

### 1.3 Command Buffer Management

**File**: `VulkanCommandBuffer.h/cpp`

- [ ] Create command pool for graphics queue
- [ ] Allocate command buffers (one per frame in flight)
- [ ] Implement BeginRecording() / EndRecording()
- [ ] Submit to queue with synchronization

**Key Class**:
```cpp
class VulkanCommandBuffer {
public:
    void Create(VkCommandPool pool);
    void Begin();
    void End();
    void Submit(VkQueue queue, VkSemaphore wait, VkSemaphore signal, VkFence fence);

    VkCommandBuffer GetHandle() const { return command_buffer_; }

private:
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
};
```

---

## Phase 2: Instanced Skeleton Rendering (Week 2)

### 2.1 Skeleton Pipeline

**Files**: `InstancedSkeletonRenderer.h/cpp`, `shaders/skeleton_instanced.vert/frag`

- [ ] Create graphics pipeline for line rendering
- [ ] Vertex input: bone start/end positions
- [ ] Instance input: per-instance transform matrix
- [ ] Uniform buffer: view-projection matrix
- [ ] Fragment shader: solid color per bone

**Vertex Shader** (`skeleton_instanced.vert`):
```glsl
#version 450

// Per-vertex data (bone line endpoints)
layout(location = 0) in vec3 in_position;

// Per-instance data
layout(location = 1) in mat4 in_instance_transform;

// Uniforms
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view_proj;
} camera;

void main() {
    gl_Position = camera.view_proj * in_instance_transform * vec4(in_position, 1.0);
}
```

**Fragment Shader** (`skeleton_instanced.frag`):
```glsl
#version 450

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(1.0, 1.0, 1.0, 1.0);  // White skeleton
}
```

**Rendering Data**:
```cpp
struct SkeletonInstanceData {
    ozz::math::Float4x4 transform;  // Per-instance world transform
};

class InstancedSkeletonRenderer {
public:
    void Initialize(VulkanDevice* device);
    void Render(VkCommandBuffer cmd,
                const ozz::animation::Skeleton& skeleton,
                const xr_vector<ozz::math::Float4x4>& instance_transforms,
                const xr_vector<ozz::math::Float4x4>& bone_matrices);

private:
    void CreatePipeline();
    void CreateVertexBuffer(const ozz::animation::Skeleton& skeleton);
    void UpdateInstanceBuffer(const xr_vector<SkeletonInstanceData>& instances);

    VulkanBuffer vertex_buffer_;  // Bone line endpoints (static)
    VulkanBuffer instance_buffer_; // Per-instance transforms (dynamic)
    VulkanBuffer uniform_buffer_;  // Camera matrices
    VkPipeline pipeline_;
    VkPipelineLayout pipeline_layout_;
};
```

**Checklist**:
- [ ] Generate bone line segments from skeleton hierarchy
- [ ] Upload to vertex buffer (one-time)
- [ ] Update instance buffer each frame
- [ ] Draw instanced: `vkCmdDrawInstanced(cmd, vertex_count, instance_count, 0, 0)`

---

## Phase 3: GPU Skinned Mesh Rendering (Week 3)

### 3.1 Skinned Mesh Pipeline

**Files**: `InstancedMeshRenderer.h/cpp`, `shaders/skinned_mesh_instanced.vert/frag`

- [ ] Create graphics pipeline for triangle rendering
- [ ] Vertex input: position, normal, uv, bone weights, bone indices
- [ ] Storage buffer: bone matrices (per-instance, per-bone)
- [ ] GPU skinning: compute skinned position/normal in vertex shader
- [ ] Fragment shader: simple shading (normal-based or textured)

**Vertex Shader** (`skinned_mesh_instanced.vert`):
```glsl
#version 450

// Per-vertex data
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_bone_weights;
layout(location = 4) in uvec4 in_bone_indices;

// Per-instance data
layout(location = 5) in mat4 in_instance_transform;
layout(location = 9) in uint in_instance_id;  // Index into bone matrix array

// Uniforms
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view_proj;
} camera;

// Storage buffer: bone matrices for all instances
layout(set = 0, binding = 1) readonly buffer BoneMatrices {
    mat4 matrices[];  // [instance_0_bone_0, bone_1, ..., instance_1_bone_0, ...]
} bones;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_uv;

void main() {
    // GPU skinning
    uint base_index = in_instance_id * MAX_BONES_PER_SKELETON;

    mat4 bone_transform =
        bones.matrices[base_index + in_bone_indices.x] * in_bone_weights.x +
        bones.matrices[base_index + in_bone_indices.y] * in_bone_weights.y +
        bones.matrices[base_index + in_bone_indices.z] * in_bone_weights.z +
        bones.matrices[base_index + in_bone_indices.w] * in_bone_weights.w;

    vec4 skinned_position = bone_transform * vec4(in_position, 1.0);
    vec4 skinned_normal = bone_transform * vec4(in_normal, 0.0);

    gl_Position = camera.view_proj * in_instance_transform * skinned_position;
    out_normal = normalize((in_instance_transform * skinned_normal).xyz);
    out_uv = in_uv;
}
```

**Fragment Shader** (`skinned_mesh_instanced.frag`):
```glsl
#version 450

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    // Simple diffuse shading
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3));
    float ndotl = max(dot(in_normal, light_dir), 0.0);
    vec3 ambient = vec3(0.2);
    vec3 diffuse = vec3(0.8) * ndotl;

    out_color = vec4(ambient + diffuse, 1.0);
}
```

**Rendering Data**:
```cpp
struct MeshInstanceData {
    ozz::math::Float4x4 transform;
    uint32_t bone_matrix_offset;  // Offset into bone storage buffer
};

class InstancedMeshRenderer {
public:
    void Initialize(VulkanDevice* device);
    void UploadMesh(const ozz::sample::Mesh& mesh);
    void Render(VkCommandBuffer cmd,
                const xr_vector<MeshInstanceData>& instances,
                const xr_vector<ozz::math::Float4x4>& all_bone_matrices);

private:
    void CreatePipeline();

    VulkanBuffer vertex_buffer_;      // Mesh vertices (static)
    VulkanBuffer index_buffer_;       // Mesh indices (static)
    VulkanBuffer instance_buffer_;    // Per-instance data (dynamic)
    VulkanBuffer bone_matrix_buffer_; // All bone matrices (dynamic, SSBO)
    VulkanBuffer uniform_buffer_;     // Camera matrices

    VkPipeline pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkDescriptorSetLayout descriptor_set_layout_;
    VkDescriptorSet descriptor_set_;
};
```

**Checklist**:
- [ ] Parse ozz::sample::Mesh into Vulkan vertex format
- [ ] Upload mesh data to GPU (one-time per mesh)
- [ ] Update bone matrices each frame (all instances concatenated)
- [ ] Update instance data each frame
- [ ] Draw instanced: `vkCmdDrawIndexedInstanced(cmd, index_count, instance_count, 0, 0, 0)`

---

## Phase 4: Debug Visualization (Week 4)

### 4.1 Debug Line Renderer

**Files**: `DebugRenderer.h/cpp`, `shaders/debug_lines.vert/frag`

- [ ] Immediate-mode line drawing API
- [ ] Dynamic vertex buffer for line batches
- [ ] Support colors per-line
- [ ] Draw IK targets, axes, collision shapes

**API**:
```cpp
class DebugRenderer {
public:
    void Begin();  // Start new frame

    void DrawLine(const ozz::math::Float3& start, const ozz::math::Float3& end,
                  const ozz::math::Float4& color);
    void DrawPoint(const ozz::math::Float3& position, float radius,
                   const ozz::math::Float4& color);
    void DrawAxes(const ozz::math::Float4x4& transform, float scale);
    void DrawSphere(const ozz::math::Float3& center, float radius,
                    const ozz::math::Float4& color);

    void End();    // Upload to GPU
    void Render(VkCommandBuffer cmd);  // Draw all batched lines

private:
    struct LineVertex {
        ozz::math::Float3 position;
        ozz::math::Float4 color;
    };

    xr_vector<LineVertex> line_vertices_;
    VulkanBuffer vertex_buffer_;  // Dynamic, recreated each frame
    VkPipeline pipeline_;
};
```

**Checklist**:
- [ ] Implement batch API
- [ ] Create line rendering pipeline (VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
- [ ] Helper functions for common shapes (sphere, box, axes)

---

## Phase 5: Integration (Week 5)

### 5.1 Replace ozz Sample Framework

**File**: `ozz_animation_viewer.cpp`

- [ ] Remove `ozz::sample::Application` dependency
- [ ] Create GLFW window manually
- [ ] Initialize `VulkanRenderer` instead of ozz renderer
- [ ] Update rendering loop to use new API
- [ ] Keep ImGui integration (render ImGui in separate render pass)

**New Main Loop**:
```cpp
int main(int argc, const char** argv) {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ozz Animation Viewer", nullptr, nullptr);

    // Initialize Vulkan renderer
    VulkanRenderer renderer;
    renderer.Initialize(window);

    // Initialize ImGui
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_Init(/* ... */);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Update ECS animation
        ecs_animation_registry_->Update(dt);

        // Begin frame
        renderer.BeginFrame();

        // Render all instances
        renderer.RenderSkeletons(instance_transforms, bone_matrices);
        renderer.RenderMeshes(instance_transforms, bone_matrices);
        renderer.RenderDebug();

        // Render ImGui
        ImGui::Render();
        renderer.RenderImGui();

        // End frame
        renderer.EndFrame();
    }

    renderer.Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```

---

## Performance Targets

- **100 instances** @ 60 FPS (1920x1080)
- **1000 instances** @ 30 FPS (1920x1080)
- **GPU memory** < 500 MB for 100 instances
- **CPU overhead** < 5% (most work on GPU)

---

## Dependencies

### Required Libraries

1. **Vulkan SDK** (1.3+)
   - Install system package or download from LunarG
   - Validation layers for debug builds

2. **GLFW** (3.3+)
   - Already in project (for window/input)

3. **VMA (Vulkan Memory Allocator)**
   - Single-header library
   - Add to `Externals/vma/`
   - Handles memory allocation/pooling

4. **glslc / shaderc** (for shader compilation)
   - Part of Vulkan SDK
   - Compile GLSL → SPIR-V at build time

5. **GLM** (optional, for math)
   - Can use ozz math types instead
   - Useful for camera/projection matrices

### Build System Changes

**CMakeLists.txt**:
```cmake
# Find Vulkan
find_package(Vulkan REQUIRED)

# Compile shaders to SPIR-V
find_program(GLSLC glslc HINTS ${Vulkan_INCLUDE_DIRS}/../bin)

function(add_shader TARGET SHADER)
    set(SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER})
    set(SHADER_SPIRV ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER}.spv)

    add_custom_command(
        OUTPUT ${SHADER_SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLC} ${SHADER_SOURCE} -o ${SHADER_SPIRV}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling shader ${SHADER}"
    )

    target_sources(${TARGET} PRIVATE ${SHADER_SPIRV})
endfunction()

# Add to ozz_animation_viewer
target_link_libraries(ozz_animation_viewer PRIVATE Vulkan::Vulkan)
add_shader(ozz_animation_viewer skeleton_instanced.vert)
add_shader(ozz_animation_viewer skeleton_instanced.frag)
add_shader(ozz_animation_viewer skinned_mesh_instanced.vert)
add_shader(ozz_animation_viewer skinned_mesh_instanced.frag)
```

---

## Testing Plan

### Unit Tests

- [ ] Vulkan device initialization
- [ ] Buffer creation/destruction
- [ ] Pipeline creation
- [ ] Command buffer recording

### Integration Tests

- [ ] Render single skeleton
- [ ] Render 10 instances
- [ ] Render 100 instances
- [ ] GPU skinning correctness (compare with CPU)
- [ ] IK visual verification

### Performance Tests

- [ ] Frame time profiling (1/10/100/1000 instances)
- [ ] GPU memory usage
- [ ] CPU overhead measurement

---

## Future Enhancements

- [ ] **PBR shading** - Physically-based materials
- [ ] **Shadow mapping** - Real-time shadows
- [ ] **Post-processing** - FXAA, bloom, tone mapping
- [ ] **Async compute** - IK solving on compute queue
- [ ] **Ray tracing** - Optional RT reflections/AO
- [ ] **Multi-GPU** - Explicit multi-GPU rendering
- [ ] **DX12 backend** - Windows-specific optimization
- [ ] **Metal backend** - macOS support

---

## Implementation Checklist

### Week 1: Foundation
- [ ] Set up Vulkan SDK
- [ ] Create VulkanDevice class
- [ ] Implement buffer abstraction
- [ ] Command buffer management
- [ ] Simple triangle test

### Week 2: Skeleton Rendering
- [ ] Skeleton vertex generation
- [ ] Instanced rendering pipeline
- [ ] Camera controller
- [ ] Render 100 static skeletons

### Week 3: Skinned Meshes
- [ ] Mesh data conversion
- [ ] GPU skinning shader
- [ ] Bone matrix upload
- [ ] Render 100 animated characters

### Week 4: Debug Visualization
- [ ] Debug line renderer
- [ ] IK target visualization
- [ ] Performance HUD

### Week 5: Integration
- [ ] Remove ozz sample dependencies
- [ ] ImGui integration
- [ ] Final polish

---

## Success Criteria

✅ **Remove all ozz sample framework code**
✅ **Render 100+ animated instances at 60 FPS**
✅ **GPU skinning working correctly**
✅ **IK visualization accurate**
✅ **Clean, maintainable Vulkan abstraction**

---

## Notes

- **WSL2 Vulkan** - Should work with WSL2 + VcXsrv or WSLg
- **Validation layers** - Always enable in debug builds
- **RenderDoc** - Use for debugging/profiling Vulkan calls
- **Nsight Graphics** - Alternative profiler (NVIDIA GPUs)

---

This design provides a complete roadmap for a high-performance, modern Vulkan renderer that will massively outperform the current ozz sample framework while giving us full control over instancing and GPU resources.
