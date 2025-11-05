#pragma once

#include <string>
#include <vector>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

namespace ozz {
namespace sample {
struct Mesh;
}
}

namespace xray {
namespace animation {
namespace tools {

// Simple OBJ loader that creates a non-skinned ozz::sample::Mesh
class SimpleObjLoader {
public:
    // Load an OBJ file into an ozz::sample::Mesh
    // Returns true on success
    static bool LoadObjFile(const std::string& file_path, ozz::sample::Mesh& out_mesh);

    // Find a resource file by searching common locations relative to CWD
    // Tries: relative path as-is, ../../../<path>, ../../<path>, ../<path>
    // Returns the resolved path if found, empty string otherwise
    static std::string FindResourceFile(const std::string& relative_path);

private:
    struct TempVertex {
        float position[3];
        float normal[3];
        float uv[2];
    };
};

} // namespace tools
} // namespace animation
} // namespace xray
