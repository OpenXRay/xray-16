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

class SimpleObjLoader {
public:
    static bool LoadObjFile(const std::string& file_path, ozz::sample::Mesh& out_mesh);

    static std::string FindResourceFile(const std::string& relative_path);

private:
    struct TempVertex {
        float position[3];
        float normal[3];
        float uv[2];
    };
};

}
}
}
