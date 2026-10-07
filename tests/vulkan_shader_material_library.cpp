#include "xrEngine/stdafx.h"
#include "src/Layers/xrRenderVK/ShaderMaterialLibrary.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace xray::render::vulkan;
using Bytes = std::vector<uint8_t>;

static void write32(Bytes& bytes, uint32_t n)
{ for (unsigned i = 0; i < 4; ++i) bytes.push_back(static_cast<uint8_t>(n >> (i * 8))); }
static void chunk(Bytes& bytes, uint32_t id, const Bytes& payload)
{ write32(bytes, id); write32(bytes, static_cast<uint32_t>(payload.size())); bytes.insert(bytes.end(), payload.begin(), payload.end()); }
static void property(Bytes& bytes, uint32_t id, const char* name, uint32_t value)
{ write32(bytes, id); bytes.insert(bytes.end(), name, name + std::strlen(name) + 1); write32(bytes, value); }
static Bytes record(const char* cls, const char* name, bool blend, uint32_t alpha_ref = 128)
{
    Bytes result(176, 0);
    for (size_t i = 0; i < 8; ++i) result[i] = static_cast<uint8_t>(cls[7 - i]);
    std::memcpy(result.data() + 8, name, std::strlen(name));
    result[172] = 1;
    property(result, 0, "General", 0); result.resize(result.size() - 4);
    write32(result, 4); result.insert(result.end(), {'P','r','i','o','r','i','t','y',0});
    write32(result, 1); write32(result, 0); write32(result, 3);
    property(result, 6, "Strict sorting", 0);
    property(result, 0, "Base Texture", 0); result.resize(result.size() - 4);
    for (const char* prop : {"Name", "Transform"})
    {
        write32(result, prop[0] == 'N' ? 3 : 1);
        result.insert(result.end(), prop, prop + std::strlen(prop) + 1);
        result.resize(result.size() + 64);
    }
    write32(result, 4); const char* alpha = "Alpha ref";
    result.insert(result.end(), alpha, alpha + std::strlen(alpha) + 1);
    write32(result, alpha_ref); write32(result, 0); write32(result, 255);
    property(result, 6, "Alpha-blend", blend ? 1 : 0);
    return result;
}

int main()
{
    Bytes list;
    chunk(list, 0, record("LM_AREF ", "def_shaders\\leaf", false));
    chunk(list, 1, record("LM_AREF ", "def_shaders\\glass", true));
    chunk(list, 2, record("D_TREE  ", "trees\\leaf", false));
    chunk(list, 3, record("D_TREE  ", "trees\\blended", true));
    chunk(list, 4, record("D_STILL ", "detail\\solid", false));
    Bytes modded = record("CUSTOM  ", "mod\\unrecognized", false);
    modded[172] = 17; // An archive can contain blender revisions newer than the engine.
    chunk(list, 5, modded);
    chunk(list, 6, record("LM_AREF ", "mod\\cutout", false, 200));
    Bytes screen_blend = record("S_SET   ", "effects\\wallmarkblend", false, 0);
    write32(screen_blend, 7);
    const char* blend_name = "Blending";
    screen_blend.insert(screen_blend.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_blend, 6); write32(screen_blend, 0);
    chunk(list, 7, screen_blend);
    Bytes screen_set = record("S_SET   ", "hud\\opaque", false, 0);
    write32(screen_set, 7);
    screen_set.insert(screen_set.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_set, 0); write32(screen_set, 0);
    chunk(list, 8, screen_set);
    Bytes file;
    chunk(file, 2, list);
    IReader reader(file.data(), file.size());
    ShaderMaterialLibrary library;
    std::string error;
    assert(library.load(reader, error) && library.size() == 9);
    SurfaceMode mode{};
    assert(library.resolve("DEF_SHADERS/LEAF", mode, error) && mode == SurfaceMode::AlphaTest);
    assert(library.resolve("def_shaders\\glass", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("trees\\leaf", mode, error) && mode == SurfaceMode::AlphaTest);
    assert(library.resolve("trees\\blended", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("detail\\solid", mode, error) && mode == SurfaceMode::Opaque);
    assert(!library.resolve("unknown", mode, error) && error.find("unknown") != std::string::npos);
    assert(!library.resolve("mod\\unrecognized", mode, error) &&
        error.find("CUSTOM") != std::string::npos && error.find("version=17") != std::string::npos);
    assert(library.resolve("mod\\cutout", mode, error) && mode == SurfaceMode::AlphaTest);
    int alpha_ref = -1;
    assert(library.resolve("mod\\cutout", mode, error, &alpha_ref) && alpha_ref == 200);
    assert(library.resolve("effects\\wallmarkblend", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("hud\\opaque", mode, error) && mode == SurfaceMode::Opaque);
    Bytes malformed, broken;
    chunk(broken, 0, Bytes(5, 0));
    chunk(malformed, 2, broken);
    IReader truncated(malformed.data(), malformed.size());
    assert(!library.load(truncated, error));
}
