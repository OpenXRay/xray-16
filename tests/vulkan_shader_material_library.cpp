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
    result[172] = std::strcmp(cls, "PARTICLE") == 0 ||
        std::strcmp(cls, "D_STILL ") == 0 ? 0 :
        std::strcmp(cls, "S_SET   ") == 0 ? 4 : 1;
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
    Bytes additive = record("S_SET   ", "effects\\additive", false, 32);
    write32(additive, 7);
    additive.insert(additive.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(additive, 2); write32(additive, 0);
    chunk(list, 9, additive);
    Bytes particle_set = record("PARTICLE", "effects\\sprite_set", false, 200);
    write32(particle_set, 7);
    particle_set.insert(particle_set.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(particle_set, 0); write32(particle_set, 0);
    chunk(list, 42, particle_set); // Modded archives may leave gaps in chunk IDs.
    Bytes screen_alpha = record("S_SET   ", "hud\\alpha", false, 64);
    write32(screen_alpha, 7);
    screen_alpha.insert(screen_alpha.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_alpha, 1); write32(screen_alpha, 0);
    chunk(list, 43, screen_alpha);
    Bytes particle_add = record("PARTICLE", "effects\\flash_add", false, 32);
    write32(particle_add, 7);
    particle_add.insert(particle_add.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(particle_add, 2); write32(particle_add, 0);
    chunk(list, 44, particle_add);
    Bytes particle_alpha_add = record("PARTICLE", "effects\\flash_alpha_add", false, 32);
    write32(particle_alpha_add, 7);
    particle_alpha_add.insert(particle_alpha_add.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(particle_alpha_add, 5); write32(particle_alpha_add, 0);
    chunk(list, 45, particle_alpha_add);
    Bytes particle_multiply = record("PARTICLE", "effects\\multiply", false, 32);
    write32(particle_multiply, 7);
    particle_multiply.insert(particle_multiply.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(particle_multiply, 3); write32(particle_multiply, 0);
    chunk(list, 46, particle_multiply);
    Bytes screen_multiply_2x = record("S_SET   ", "effects\\screen_multiply_2x", false, 32);
    write32(screen_multiply_2x, 7);
    screen_multiply_2x.insert(screen_multiply_2x.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_multiply_2x, 4); write32(screen_multiply_2x, 0);
    chunk(list, 47, screen_multiply_2x);
    Bytes wallmark_mult = record("S_SET   ", "effects\\wallmarkmult", false, 0);
    write32(wallmark_mult, 7);
    wallmark_mult.insert(wallmark_mult.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(wallmark_mult, 6); write32(wallmark_mult, 0);
    property(wallmark_mult, 6, "Z-test", 1);
    chunk(list, 51, wallmark_mult);
    chunk(list, 52, record("MODEL   ", "models\\fur", true, 32));
    chunk(list, 53, record("MODEL   ", "models\\glass", true, 4));
    Bytes file;
    chunk(file, 2, list);
    IReader reader(file.data(), file.size());
    ShaderMaterialLibrary library;
    std::string error;
    assert(library.load(reader, error) && library.size() == 19);
    assert(library.contains("HUD/ALPHA"));
    assert(!library.contains("hud\\font")); // Built-in UI shaders are not serialized blenders.
    SurfaceMode mode{};
    assert(library.resolve("DEF_SHADERS/LEAF", mode, error) && mode == SurfaceMode::AlphaTest);
    assert(library.resolve("def_shaders\\glass", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("trees\\leaf", mode, error) && mode == SurfaceMode::AlphaTest);
    assert(library.resolve("trees\\blended", mode, error) && mode == SurfaceMode::AlphaTest);
    assert(library.resolve("models\\fur", mode, error) && mode == SurfaceMode::AlphaTest);
    assert(library.resolve("models\\glass", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("detail\\solid", mode, error) && mode == SurfaceMode::Opaque);
    assert(!library.resolve("unknown", mode, error) && error.find("unknown") != std::string::npos);
    assert(!library.resolve("mod\\unrecognized", mode, error) &&
        error.find("shaders.xr blender id=5") != std::string::npos &&
        error.find("CUSTOM") != std::string::npos && error.find("version=17") != std::string::npos);
    assert(library.resolve("mod\\cutout", mode, error) && mode == SurfaceMode::AlphaTest);
    int alpha_ref = -1;
    assert(library.resolve("mod\\cutout", mode, error, &alpha_ref) && alpha_ref == 200);
    assert(!library.resolve("effects\\wallmarkblend", mode, error) &&
        error.find("blending=6") != std::string::npos && error.find("id=7") != std::string::npos);
    assert(library.resolve("hud\\opaque", mode, error) && mode == SurfaceMode::Opaque);
    assert(library.resolve("hud\\alpha", mode, error) && mode == SurfaceMode::Transparent);
    assert(!library.resolve("effects\\additive", mode, error) &&
        error.find("blending=2") != std::string::npos && error.find("id=9") != std::string::npos);
    int screen_particle_blend = -1;
    assert(library.resolve("effects\\additive", mode, error, nullptr,
        &screen_particle_blend, true) && mode == SurfaceMode::Transparent && screen_particle_blend == 2);
    assert(library.resolve("effects\\sprite_set", mode, error, &alpha_ref) &&
        mode == SurfaceMode::AlphaTest && alpha_ref == 200);
    int blend_mode = -1;
    assert(library.resolve("effects\\flash_add", mode, error, nullptr, &blend_mode) &&
        mode == SurfaceMode::Transparent && blend_mode == 2);
    assert(library.resolve("effects\\flash_alpha_add", mode, error, nullptr, &blend_mode) &&
        mode == SurfaceMode::Transparent && blend_mode == 5);
    assert(!library.resolve("effects\\multiply", mode, error) &&
        error.find("blending=3") != std::string::npos);
    assert(library.resolve("effects\\multiply", mode, error, nullptr, &blend_mode, true) &&
        mode == SurfaceMode::Transparent && blend_mode == 3);
    assert(!library.resolve("effects\\screen_multiply_2x", mode, error) &&
        error.find("blending=4") != std::string::npos);
    assert(library.resolve("effects\\screen_multiply_2x", mode, error, nullptr, &blend_mode, true) &&
        mode == SurfaceMode::Transparent && blend_mode == 4);
    assert(library.resolve("effects\\wallmarkmult", mode, error, nullptr, &blend_mode) &&
        mode == SurfaceMode::Transparent && blend_mode == 6);
    assert(!library.resolve("effects\\wallmarkmult", mode, error, nullptr, &blend_mode, true) &&
        error.find("blending=6") != std::string::npos);
    assert(!library.resolve("effects\\wallmarkmult", mode, error, nullptr, &blend_mode, false, true) &&
        error.find("Z-test") != std::string::npos);
    for (const auto& name : {"hud\\opaque", "hud\\alpha", "effects\\additive",
             "effects\\wallmarkblend", "effects\\screen_multiply_2x"})
    {
        assert(library.resolve(name, mode, error, &alpha_ref, &blend_mode, false, true));
        assert(blend_mode >= 0 && blend_mode <= 9);
    }
    assert(!library.resolve("trees\\leaf", mode, error, nullptr, nullptr, false, true) &&
        error.find("screen class") != std::string::npos);
    Bytes screen_depth = record("S_SET   ", "mod\\depth_ui", false);
    write32(screen_depth, 7);
    screen_depth.insert(screen_depth.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_depth, 1); write32(screen_depth, 0);
    property(screen_depth, 6, "Z-test", 1);
    Bytes depth_list, depth_file;
    chunk(depth_list, 48, screen_depth);
    chunk(depth_file, 2, depth_list);
    IReader depth_reader(depth_file.data(), depth_file.size());
    assert(library.load(depth_reader, error));
    assert(library.resolve("mod\\depth_ui", mode, error, nullptr, &blend_mode, true) &&
        mode == SurfaceMode::Transparent);
    assert(!library.resolve("mod\\depth_ui", mode, error, nullptr, &blend_mode, false, true) &&
        error.find("Z-test") != std::string::npos && error.find("id=48") != std::string::npos);
    Bytes depth_write = record("S_SET   ", "mod\\depth_write", false);
    write32(depth_write, 7);
    depth_write.insert(depth_write.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(depth_write, 1); write32(depth_write, 0);
    property(depth_write, 6, "Z-test", 1);
    property(depth_write, 6, "Z-write", 1);
    Bytes depth_write_list, depth_write_file;
    chunk(depth_write_list, 50, depth_write);
    chunk(depth_write_file, 2, depth_write_list);
    IReader depth_write_reader(depth_write_file.data(), depth_write_file.size());
    assert(library.load(depth_write_reader, error));
    assert(!library.resolve("mod\\depth_write", mode, error, nullptr, &blend_mode, true) &&
        error.find("Z-write") != std::string::npos);
    Bytes screen_wrap = record("S_SET   ", "mod\\wrap_ui", false);
    write32(screen_wrap, 7);
    screen_wrap.insert(screen_wrap.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_wrap, 1); write32(screen_wrap, 0);
    property(screen_wrap, 6, "Texture clamp", 0);
    Bytes wrap_list, wrap_file;
    chunk(wrap_list, 49, screen_wrap);
    chunk(wrap_file, 2, wrap_list);
    IReader wrap_reader(wrap_file.data(), wrap_file.size());
    assert(library.load(wrap_reader, error));
    assert(library.resolve("mod\\wrap_ui", mode, error)); // Scene sampler repeats.
    assert(!library.resolve("mod\\wrap_ui", mode, error, nullptr, nullptr, false, true) &&
        error.find("Texture clamp") != std::string::npos && error.find("id=49") != std::string::npos);
    Bytes water = record("S_SET   ", "effects\\water", false, 0);
    write32(water, 7); water.insert(water.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(water, 0); write32(water, 0);
    property(water, 6, "Strict sorting", 1);
    property(water, 6, "Texture clamp", 0);
    property(water, 6, "Z-test", 1);
    property(water, 6, "Z-write", 1);
    property(water, 6, "Lighting", 1);
    Bytes lod = record("S_SET   ", "details\\lod", false, 16);
    write32(lod, 7); lod.insert(lod.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(lod, 1); write32(lod, 0);
    property(lod, 6, "Texture clamp", 0);
    property(lod, 6, "Z-test", 1);
    property(lod, 6, "Fog", 1);
    Bytes glow = record("S_SET   ", "effects\\glow", false, 0);
    write32(glow, 7); glow.insert(glow.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(glow, 5); write32(glow, 0);
    property(glow, 6, "Z-test", 1);
    Bytes level_list, level_file;
    chunk(level_list, 52, water); chunk(level_list, 25, lod); chunk(level_list, 36, glow);
    chunk(level_file, 2, level_list);
    IReader level_reader(level_file.data(), level_file.size());
    assert(library.load(level_reader, error));
    uint32_t flags = 0;
    assert(!library.resolve("effects\\water", mode, error, nullptr, nullptr, false, false, true) &&
        error.find("Z-write") != std::string::npos);
    assert(library.resolve("effects\\water", mode, error, nullptr, &blend_mode,
        false, false, true, true, &flags) && mode == SurfaceMode::Transparent &&
        blend_mode == 0 && (flags & 15u) == 15u);
    assert(!library.resolve("details\\lod", mode, error, nullptr, nullptr, false, false, false) &&
        error.find("Fog") != std::string::npos);
    assert(library.resolve("details\\lod", mode, error, nullptr, &blend_mode,
        false, false, true, false, &flags) && mode == SurfaceMode::Transparent &&
        blend_mode == 1 && (flags & 16u));
    assert(!library.resolve("effects\\glow", mode, error) &&
        error.find("blending=5") != std::string::npos);
    assert(library.resolve("effects\\glow", mode, error, nullptr, &blend_mode,
        false, false, true) && mode == SurfaceMode::Transparent && blend_mode == 5);
    Bytes newer = record("LM_AREF ", "mod\\future", false);
    newer[172] = 42;
    Bytes future_list, future_file;
    chunk(future_list, 19, newer);
    chunk(future_file, 2, future_list);
    IReader future(future_file.data(), future_file.size());
    assert(library.load(future, error));
    assert(!library.resolve("mod\\future", mode, error) &&
        error.find("id=19") != std::string::npos &&
        error.find("version=42") != std::string::npos);
    Bytes invalid_alpha = record("LM_AREF ", "mod\\bad_aref", false, 256);
    Bytes alpha_list, alpha_file;
    chunk(alpha_list, 20, invalid_alpha);
    chunk(alpha_file, 2, alpha_list);
    IReader bad_alpha(alpha_file.data(), alpha_file.size());
    assert(!library.load(bad_alpha, error) && error.find("Alpha ref") != std::string::npos &&
        error.find("id=20") != std::string::npos);
    Bytes missing_blending = record("PARTICLE", "mod\\missing_blend", false);
    Bytes missing_list, missing_file;
    chunk(missing_list, 21, missing_blending);
    chunk(missing_file, 2, missing_list);
    IReader missing(missing_file.data(), missing_file.size());
    assert(!library.load(missing, error) && error.find("Blending") != std::string::npos &&
        error.find("id=21") != std::string::npos);
    Bytes invalid_token = record("S_SET   ", "mod\\bad_token", false);
    write32(invalid_token, 7);
    invalid_token.insert(invalid_token.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(invalid_token, 2); write32(invalid_token, 1);
    write32(invalid_token, 1); invalid_token.resize(invalid_token.size() + 64);
    Bytes token_list, token_file;
    chunk(token_list, 22, invalid_token);
    chunk(token_file, 2, token_list);
    IReader bad_token(token_file.data(), token_file.size());
    assert(!library.load(bad_token, error) && error.find("Blending token") != std::string::npos &&
        error.find("id=22") != std::string::npos);
    Bytes malformed, broken;
    chunk(broken, 0, Bytes(5, 0));
    chunk(malformed, 2, broken);
    IReader truncated(malformed.data(), malformed.size());
    assert(!library.load(truncated, error));
    assert(error.find("shaders.xr blender id=0") != std::string::npos);
    Bytes wrong_property = record("LM      ", "invalid\\property", false);
    write32(wrong_property, 99);
    wrong_property.insert(wrong_property.end(), {'M','o','d',0});
    Bytes invalid_list, invalid_file;
    chunk(invalid_list, 0, wrong_property);
    chunk(invalid_file, 2, invalid_list);
    IReader unknown_property(invalid_file.data(), invalid_file.size());
    assert(!library.load(unknown_property, error));
    assert(error.find("type=99") != std::string::npos &&
        error.find("invalid\\property") != std::string::npos);
    Bytes duplicate_list, duplicate_file;
    chunk(duplicate_list, 0, record("LM      ", "duplicate", false));
    chunk(duplicate_list, 1, record("LM      ", "DUPLICATE", false));
    chunk(duplicate_file, 2, duplicate_list);
    IReader duplicate(duplicate_file.data(), duplicate_file.size());
    assert(!library.load(duplicate, error) && error.find("blender id=1") != std::string::npos);
    Bytes oversized_file;
    Bytes oversized_list;
    write32(oversized_list, 7); write32(oversized_list, 9999);
    chunk(oversized_file, 2, oversized_list);
    IReader oversized(oversized_file.data(), oversized_file.size());
    assert(!library.load(oversized, error) && error.find("truncated chunk") != std::string::npos);
}
