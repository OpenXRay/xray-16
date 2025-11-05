#include "stdafx.h"
#include "SimpleObjLoader.h"
#include "framework/mesh.h"

#include <fstream>
#include <sstream>
#include <cstdio>
#include <unordered_map>
#include <cstring>
#include <filesystem>

#define Msg(...) printf(__VA_ARGS__), printf("\n")

namespace xray {
namespace animation {
namespace tools {

// Hash function for vertex deduplication
struct VertexKey {
    int v_idx, vt_idx, vn_idx;

    bool operator==(const VertexKey& other) const {
        return v_idx == other.v_idx && vt_idx == other.vt_idx && vn_idx == other.vn_idx;
    }
};

} // namespace tools
} // namespace animation
} // namespace xray

namespace std {
template <>
struct hash<xray::animation::tools::VertexKey> {
    size_t operator()(const xray::animation::tools::VertexKey& k) const {
        return ((hash<int>()(k.v_idx) ^ (hash<int>()(k.vt_idx) << 1)) >> 1) ^ (hash<int>()(k.vn_idx) << 1);
    }
};
}

namespace xray {
namespace animation {
namespace tools {

std::string SimpleObjLoader::FindResourceFile(const std::string& relative_path) {
    // List of search paths to try (in order)
    const std::vector<std::string> search_prefixes = {
        "",           // Try as-is first (for Linux builds from project root)
        "../../../",  // Visual Studio (src/xrAnimation/tools -> project root)
        "../../",     // Some build configs
        "../",        // Other build configs
    };

    for (const auto& prefix : search_prefixes) {
        std::string candidate = prefix + relative_path;

        // Check if file exists
        if (std::filesystem::exists(candidate)) {
            auto abs_path = std::filesystem::absolute(candidate);
            Msg("* Found resource: %s -> %s", relative_path.c_str(), abs_path.string().c_str());
            return candidate;
        }
    }

    // Not found in any location
    Msg("! Resource not found: %s (tried %zu locations)", relative_path.c_str(), search_prefixes.size());
    auto cwd = std::filesystem::current_path();
    Msg("! CWD: %s", cwd.string().c_str());

    return "";
}

bool SimpleObjLoader::LoadObjFile(const std::string& file_path, ozz::sample::Mesh& out_mesh) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        Msg("! Failed to open OBJ file: %s", file_path.c_str());
        return false;
    }

    // Temporary storage for OBJ data (1-indexed, so we add dummy at index 0)
    std::vector<float> positions;  // x,y,z
    std::vector<float> texcoords;  // u,v
    std::vector<float> normals;    // x,y,z

    // Add dummy elements at index 0
    positions.push_back(0); positions.push_back(0); positions.push_back(0);
    texcoords.push_back(0); texcoords.push_back(0);
    normals.push_back(0); normals.push_back(0); normals.push_back(0);

    // Final mesh data
    std::vector<float> mesh_positions;
    std::vector<float> mesh_normals;
    std::vector<float> mesh_uvs;
    std::vector<uint32_t> mesh_indices;

    // Vertex deduplication
    std::unordered_map<VertexKey, uint32_t> vertex_map;

    std::string line;
    int line_number = 0;

    while (std::getline(file, line)) {
        line_number++;

        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            // Vertex position
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        }
        else if (prefix == "vt") {
            // Texture coordinate
            float u, v;
            iss >> u >> v;
            texcoords.push_back(u);
            texcoords.push_back(v);
        }
        else if (prefix == "vn") {
            // Normal
            float nx, ny, nz;
            iss >> nx >> ny >> nz;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
        }
        else if (prefix == "f") {
            // Face (triangle)
            std::vector<VertexKey> face_vertices;
            std::string vertex_str;

            while (iss >> vertex_str) {
                VertexKey key{};

                // Parse vertex format: v/vt/vn or v//vn or v/vt or v
                int v_idx = 0, vt_idx = 0, vn_idx = 0;

                size_t pos1 = vertex_str.find('/');
                if (pos1 == std::string::npos) {
                    // Format: v
                    v_idx = std::stoi(vertex_str);
                } else {
                    v_idx = std::stoi(vertex_str.substr(0, pos1));

                    size_t pos2 = vertex_str.find('/', pos1 + 1);
                    if (pos2 == std::string::npos) {
                        // Format: v/vt
                        vt_idx = std::stoi(vertex_str.substr(pos1 + 1));
                    } else {
                        // Format: v/vt/vn or v//vn
                        if (pos2 > pos1 + 1) {
                            vt_idx = std::stoi(vertex_str.substr(pos1 + 1, pos2 - pos1 - 1));
                        }
                        vn_idx = std::stoi(vertex_str.substr(pos2 + 1));
                    }
                }

                // Convert negative indices to positive
                if (v_idx < 0) v_idx = static_cast<int>(positions.size() / 3) + v_idx;
                if (vt_idx < 0) vt_idx = static_cast<int>(texcoords.size() / 2) + vt_idx;
                if (vn_idx < 0) vn_idx = static_cast<int>(normals.size() / 3) + vn_idx;

                key.v_idx = v_idx;
                key.vt_idx = vt_idx;
                key.vn_idx = vn_idx;

                face_vertices.push_back(key);
            }

            // Triangulate if needed (fan triangulation for n>3)
            for (size_t i = 1; i + 1 < face_vertices.size(); ++i) {
                const size_t triangle_indices[3] = {0, i, i + 1};
                for (size_t idx = 0; idx < 3; ++idx) {
                    const size_t j = triangle_indices[idx];
                    const VertexKey& key = face_vertices[j];

                    auto it = vertex_map.find(key);
                    if (it != vertex_map.end()) {
                        // Reuse existing vertex
                        mesh_indices.push_back(it->second);
                    } else {
                        // Add new vertex
                        uint32_t new_index = static_cast<uint32_t>(mesh_positions.size() / 3);
                        vertex_map[key] = new_index;
                        mesh_indices.push_back(new_index);

                        // Add position
                        if (key.v_idx > 0 && key.v_idx * 3 < static_cast<int>(positions.size())) {
                            mesh_positions.push_back(positions[key.v_idx * 3 + 0]);
                            mesh_positions.push_back(positions[key.v_idx * 3 + 1]);
                            mesh_positions.push_back(positions[key.v_idx * 3 + 2]);
                        } else {
                            mesh_positions.push_back(0); mesh_positions.push_back(0); mesh_positions.push_back(0);
                        }

                        // Add UV
                        if (key.vt_idx > 0 && key.vt_idx * 2 < static_cast<int>(texcoords.size())) {
                            mesh_uvs.push_back(texcoords[key.vt_idx * 2 + 0]);
                            mesh_uvs.push_back(texcoords[key.vt_idx * 2 + 1]);
                        } else {
                            mesh_uvs.push_back(0); mesh_uvs.push_back(0);
                        }

                        // Add normal
                        if (key.vn_idx > 0 && key.vn_idx * 3 < static_cast<int>(normals.size())) {
                            mesh_normals.push_back(normals[key.vn_idx * 3 + 0]);
                            mesh_normals.push_back(normals[key.vn_idx * 3 + 1]);
                            mesh_normals.push_back(normals[key.vn_idx * 3 + 2]);
                        } else {
                            mesh_normals.push_back(0); mesh_normals.push_back(0); mesh_normals.push_back(1);
                        }
                    }
                }
            }
        }
    }

    file.close();

    if (mesh_positions.empty() || mesh_indices.empty()) {
        Msg("! OBJ file contained no geometry: %s", file_path.c_str());
        return false;
    }

    // Convert uint32_t indices to uint16_t for ozz
    out_mesh.triangle_indices.clear();
    out_mesh.triangle_indices.reserve(mesh_indices.size());
    for (uint32_t idx : mesh_indices) {
        if (idx > 65535) {
            Msg("! Warning: OBJ has too many vertices for uint16_t indices");
        }
        out_mesh.triangle_indices.push_back(static_cast<uint16_t>(idx));
    }

    // Create a single part with all vertices (no skinning)
    out_mesh.parts.resize(1);
    ozz::sample::Mesh::Part& part = out_mesh.parts[0];

    part.positions.assign(mesh_positions.begin(), mesh_positions.end());
    part.normals.assign(mesh_normals.begin(), mesh_normals.end());
    part.uvs.assign(mesh_uvs.begin(), mesh_uvs.end());

    // No skinning data
    out_mesh.joint_remaps.clear();
    out_mesh.inverse_bind_poses.clear();

    Msg("* Loaded OBJ: %zu vertices, %zu triangles",
        mesh_positions.size() / 3, mesh_indices.size() / 3);

    return true;
}

} // namespace tools
} // namespace animation
} // namespace xray
