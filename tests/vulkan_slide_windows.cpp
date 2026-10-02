#include "src/Layers/xrRenderVK/SlidingWindows.h"
#include "src/Layers/xrRenderVK/ModelGeometry.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <cstring>

using namespace xray::render::vulkan;

static void u16(std::vector<uint8_t>& bytes, uint16_t value)
{
    bytes.push_back(uint8_t(value)); bytes.push_back(uint8_t(value >> 8));
}
static void u32(std::vector<uint8_t>& bytes, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) bytes.push_back(uint8_t(value >> (8 * i)));
}
static void chunk(std::vector<uint8_t>& bytes, uint32_t id, const std::vector<uint8_t>& body)
{
    u32(bytes, id); u32(bytes, static_cast<uint32_t>(body.size()));
    bytes.insert(bytes.end(), body.begin(), body.end());
}
static void f32(std::vector<uint8_t>& bytes, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    u32(bytes, bits);
}

int main()
{
    const std::vector<uint32_t> indices{0, 1, 2, 0, 2, 3, 0, 1, 2};
    std::vector<uint8_t> source;
    for (unsigned i = 0; i < 4; ++i) u32(source, 0);
    u32(source, 2);
    u32(source, 0); u16(source, 2); u16(source, 4);
    u32(source, 6); u16(source, 1); u16(source, 3);
    std::vector<SlideWindow> windows;
    std::string error;
    assert(decode_slide_windows(source.data(), source.size(), indices, 4, windows, error));
    assert(windows.size() == 2 && windows[0].index_count == 6 && windows[1].offset == 6);
    assert(select_slide_window(windows, lod_for_distance(1.f, 1.f), indices.size()).index_count == 6);
    assert(select_slide_window(windows, lod_for_distance(1.f, 10000.f), indices.size()).offset == 6);
    assert(select_slide_window(windows, -1.f, indices.size()).offset == 6);
    assert(select_slide_window(windows, std::numeric_limits<float>::quiet_NaN(), indices.size()).offset == 0);
    assert(select_slide_window({}, 0.f, indices.size()).index_count == indices.size());
    assert(lod_for_distance(0.f, 10000.f) == 1.f);

    source[20 + 8] = 8; // The second window extends past the index buffer.
    assert(!decode_slide_windows(source.data(), source.size(), indices, 4, windows, error));
    assert(windows.size() == 2);
    source[20 + 8] = 6;
    source[20 + 8 + 6] = 2; // Index 2 is now outside the active vertex count.
    assert(!decode_slide_windows(source.data(), source.size(), indices, 4, windows, error));
    assert(windows.size() == 2);

    // A progressive child in an OGF hierarchy keeps every index and window.
    std::vector<uint8_t> vertices, serialized_indices;
    u32(vertices, 0x112); u32(vertices, 3);
    for (unsigned vertex = 0; vertex < 3; ++vertex)
        for (unsigned component = 0; component < 8; ++component)
            f32(vertices, component == 0 ? float(vertex) : 0.f);
    u32(serialized_indices, 6);
    for (uint16_t index : {0, 1, 2, 0, 1, 2}) u16(serialized_indices, index);
    std::vector<uint8_t> serialized_windows;
    for (unsigned i = 0; i < 4; ++i) u32(serialized_windows, 0);
    u32(serialized_windows, 2);
    u32(serialized_windows, 0); u16(serialized_windows, 2); u16(serialized_windows, 3);
    u32(serialized_windows, 3); u16(serialized_windows, 1); u16(serialized_windows, 3);
    VisualRecord child;
    child.type = 2;
    chunk(child.source, 3, vertices);
    chunk(child.source, 4, serialized_indices);
    chunk(child.source, 6, serialized_windows);
    VisualRecord parent;
    parent.type = 1;
    parent.embedded_children.push_back(std::move(child));
    ModelGeometry geometry;
    assert(decode_model_geometry(parent, geometry, error));
    assert(geometry.children.size() == 1 && geometry.children[0].indices.size() == 6);
    assert(select_slide_window(geometry.children[0].windows, 0.f, 6).offset == 3);
}
