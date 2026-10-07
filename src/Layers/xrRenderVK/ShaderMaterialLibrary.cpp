#include "xrEngine/stdafx.h"
#include "ShaderMaterialLibrary.h"
#include "xrCore/stream_reader.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>

namespace xray::render::vulkan
{
namespace
{
uint32_t le32(const uint8_t* p)
{ return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }

struct Cursor
{
    const uint8_t* bytes{};
    size_t size{}, offset{};
    bool skip(size_t n) { if (n > size - offset) return false; offset += n; return true; }
    bool string(std::string& out)
    {
        if (offset == size) return false;
        const auto* end = static_cast<const uint8_t*>(std::memchr(bytes + offset, 0, size - offset));
        if (!end) return false;
        out.assign(reinterpret_cast<const char*>(bytes + offset), end - (bytes + offset));
        offset += out.size() + 1;
        return true;
    }
    bool u32(uint32_t& n) { if (size - offset < 4) return false; n = le32(bytes + offset); offset += 4; return true; }
};

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
        { return static_cast<char>(std::tolower(c == '/' ? '\\' : c)); });
    return value;
}

bool parse_record(const uint8_t* bytes, size_t size, std::string& name,
    SurfaceMode& mode, bool& supported, std::string& cls_name,
    uint16_t& version, int& alpha_ref, std::string& error)
{
    // CBlender_DESC is pack(4): CLASS_ID[8], cName[128], cComputer[32],
    // cTime[4], version[2], trailing padding[2]. All fields are little endian.
    if (!bytes || size < 176 || !std::memchr(bytes + 8, 0, 128))
    { error = "invalid shaders.xr blender description"; return false; }
    name = lower(std::string(reinterpret_cast<const char*>(bytes + 8)));
    cls_name.assign(reinterpret_cast<const char*>(bytes), 8);
    std::reverse(cls_name.begin(), cls_name.end());
    version = uint16_t(bytes[172]) | uint16_t(bytes[173]) << 8;
    Cursor cursor{bytes, size, 176};
    bool blend = false, strict = false;
    int screen_blend = -1;
    alpha_ref = -1;
    while (cursor.offset < cursor.size)
    {
        uint32_t type;
        std::string property;
        if (!cursor.u32(type) || !cursor.string(property))
        { error = "truncated shaders.xr property in '" + name + "'"; return false; }
        const size_t start = cursor.offset;
        size_t length = 0;
        switch (type)
        {
        case 0: break;
        case 11: length = 8; break; // xrP_Template
        // xrPWRITE_PROP stores the string64 value after the property name.
        case 1: case 2: case 3: case 9: case 10: length = 64; break;
        case 4: case 5: length = 12; break;
        case 6: length = 4; break;
        case 7: case 8:
        {
            if (cursor.size - start < 8) { error = "truncated blender token"; return false; }
            const uint32_t count = le32(bytes + start + 4);
            if (count > 4096) { error = "oversized blender token"; return false; }
            length = (type == 7 ? 8 : 12) + size_t(count) * (type == 7 ? 68 : 8);
            break;
        }
        default: error = "unknown shaders.xr property type=" + std::to_string(type) + " in '" + name + "'"; return false;
        }
        if (!cursor.skip(length)) { error = "truncated blender property in '" + name + "'"; return false; }
        if ((type == 7 || type == 8) && property == "Blending")
            screen_blend = static_cast<int>(le32(bytes + start));
        if (type == 6 && property == "Strict sorting") strict = le32(bytes + start) != 0;
        if (type == 6 && (property == "Alpha-blend" || property == "Alpha-Blend" || property == "Use alpha-channel"))
            blend = le32(bytes + start) != 0;
        if (type == 4 && property == "Alpha ref") alpha_ref = static_cast<int32_t>(le32(bytes + start));
    }
    const std::string& cls = cls_name;
    supported = cls == "LM      " || cls == "LM_AREF " || cls == "V       " || cls == "V_AREF  " ||
        cls == "D_TREE  " || cls == "D_STILL " || cls == "MODEL   " || cls == "MODELEbB" ||
        cls == "PARTICLE" || cls == "LmBmmD  " || cls == "LaEmB   " ||
        cls == "LmEbB   " || cls == "BmmD    " || cls == "BmmDold " || cls == "S_SET   ";
    if (cls == "PARTICLE" || strict || (cls == "S_SET   " && screen_blend != 0) ||
        ((cls == "LM_AREF " || cls == "V_AREF  " || cls == "MODEL   " ||
            cls == "MODELEbB" || cls == "D_TREE  ") && blend))
        mode = SurfaceMode::Transparent;
    else if (cls == "LM_AREF " || cls == "V_AREF  " || cls == "D_TREE  " ||
        (cls == "D_STILL " && blend))
        mode = SurfaceMode::AlphaTest;
    else
        mode = SurfaceMode::Opaque;
    return true;
}
}

bool ShaderMaterialLibrary::load(IReader& file, std::string& error)
{
    clear();
    std::unique_ptr<IReader, void(*)(IReader*)> blenders(file.open_chunk(2), [](IReader* p) { if (p) p->close(); });
    if (!blenders) { error = "shaders.xr has no blender chunk"; return false; }
    for (uint32_t id = 0;; ++id)
    {
        std::unique_ptr<IReader, void(*)(IReader*)> record(blenders->open_chunk(id), [](IReader* p) { if (p) p->close(); });
        if (!record) break;
        std::string name, cls;
        SurfaceMode mode{};
        bool supported = false;
        uint16_t version = 0;
        int alpha_ref = -1;
        if (!parse_record(static_cast<const uint8_t*>(record->pointer()), record->length(),
                name, mode, supported, cls, version, alpha_ref, error) || name.empty() ||
            !entries_.emplace(name, Entry{mode, cls, supported, version, alpha_ref}).second)
        {
            if (error.empty()) error = "duplicate or empty shaders.xr blender name: " + name;
            error = "shaders.xr blender id=" + std::to_string(id) + ": " + error;
            clear();
            return false;
        }
        Msg("[renderer-vulkan] material.definition id=%u name='%s' class='%s' version=%u mode=%s alpha-ref=%d supported=%d",
            id, name.c_str(), cls.c_str(), version,
            mode == SurfaceMode::Transparent ? "transparent" : mode == SurfaceMode::AlphaTest ? "cutout" : "opaque",
            alpha_ref, supported ? 1 : 0);
    }
    if (entries_.empty()) { error = "shaders.xr contains no blender definitions"; return false; }
    error.clear();
    return true;
}

bool ShaderMaterialLibrary::resolve(const std::string& shader, SurfaceMode& mode,
    std::string& error, int* alpha_ref) const
{
    const auto it = entries_.find(lower(shader));
    if (it == entries_.end())
    { error = "shader '" + shader + "' not found in shaders.xr"; return false; }
    if (!it->second.supported)
    { error = "unsupported blender class '" + it->second.class_name + "' version=" +
        std::to_string(it->second.version) + " for shader '" + shader + "'"; return false; }
    mode = it->second.mode;
    if (alpha_ref) *alpha_ref = it->second.alpha_ref >= 0 ?
        std::clamp(it->second.alpha_ref, 0, 255) : 128;
    error.clear();
    return true;
}
}
