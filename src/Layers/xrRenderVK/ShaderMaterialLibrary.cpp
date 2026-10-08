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
    uint16_t& version, int& alpha_ref, int& blending, std::string& screen_issue,
    std::string& error)
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
    bool has_blending = false;
    blending = -1;
    alpha_ref = -1;
    screen_issue.clear();
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
        {
            blending = static_cast<int>(le32(bytes + start));
            has_blending = true;
            const uint32_t count = le32(bytes + start + 4);
            if (count)
            {
                bool selected = false;
                const size_t item_size = type == 7 ? 68 : 8;
                const size_t header_size = type == 7 ? 8 : 12;
                for (uint32_t i = 0; i < count; ++i)
                    selected |= le32(bytes + start + header_size + size_t(i) * item_size) ==
                        static_cast<uint32_t>(blending);
                if (!selected)
                { error = "invalid Blending token selection in '" + name + "'"; return false; }
            }
        }
        if (type == 6 && property == "Strict sorting") strict = le32(bytes + start) != 0;
        if (type == 6 &&
            ((property == "Texture clamp" && !le32(bytes + start)) ||
             ((property == "Z-test" || property == "Z-write" ||
               property == "Lighting" || property == "Fog") && le32(bytes + start))) &&
            screen_issue.empty())
            screen_issue = property;
        if (type == 6 && (property == "Alpha-blend" || property == "Alpha-Blend" || property == "Use alpha-channel"))
            blend = le32(bytes + start) != 0;
        if (type == 4 && property == "Alpha ref")
        {
            alpha_ref = static_cast<int32_t>(le32(bytes + start));
            if (alpha_ref < 0 || alpha_ref > 255)
            { error = "invalid Alpha ref in '" + name + "'"; return false; }
        }
    }
    const std::string& cls = cls_name;
    supported = cls == "LM      " || cls == "LM_AREF " || cls == "V       " || cls == "V_AREF  " ||
        cls == "D_TREE  " || cls == "D_STILL " || cls == "MODEL   " || cls == "MODELEbB" ||
        cls == "PARTICLE" || cls == "LmBmmD  " || cls == "LaEmB   " ||
        cls == "LmEbB   " || cls == "BmmD    " || cls == "BmmDold " || cls == "S_SET   ";
    // A known class with a newer serialized layout is just as unsafe as an
    // unknown class: its properties may have different pass semantics.
    const uint16_t max_version = cls == "S_SET   " ? 4 :
        cls == "BmmD    " || cls == "BmmDold " ? 3 :
        cls == "MODEL   " ? 2 :
        cls == "PARTICLE" || cls == "D_STILL " || cls == "LaEmB   " ? 0 : 1;
    supported &= version <= max_version;
    if ((cls == "PARTICLE" || cls == "S_SET   ") && !has_blending)
    { error = "missing Blending property in '" + name + "'"; return false; }
    if ((cls == "PARTICLE" && blending != 0) || strict || (cls == "S_SET   " && blending != 0) ||
        ((cls == "LM_AREF " || cls == "V_AREF  " || cls == "MODEL   " ||
            cls == "MODELEbB" || cls == "D_TREE  ") && blend))
        mode = SurfaceMode::Transparent;
    else if ((cls == "PARTICLE" && blending == 0) || cls == "LM_AREF " || cls == "V_AREF  " || cls == "D_TREE  " ||
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
    error.clear();
    std::unique_ptr<IReader, void(*)(IReader*)> blenders(file.open_chunk(2), [](IReader* p) { if (p) p->close(); });
    if (!blenders) { error = "shaders.xr has no blender chunk"; return false; }
    // open_chunk_iterator trusts chunk lengths. Validate the entire table first
    // so a damaged mod archive cannot make the iterator step beyond the buffer.
    Cursor table{static_cast<const uint8_t*>(blenders->pointer()), blenders->length(), 0};
    while (table.offset < table.size)
    {
        uint32_t id, length;
        if (!table.u32(id) || !table.u32(length) || !table.skip(length))
        { error = "shaders.xr blender table has a truncated chunk"; return false; }
    }
    uint32_t id = 0;
    for (IReader* record = blenders->open_chunk_iterator(id); record;
        record = blenders->open_chunk_iterator(id, record))
    {
        std::string name, cls;
        SurfaceMode mode{};
        bool supported = false;
        uint16_t version = 0;
        int alpha_ref = -1, blending = -1;
        std::string screen_issue;
        if (!parse_record(static_cast<const uint8_t*>(record->pointer()), record->length(),
                name, mode, supported, cls, version, alpha_ref, blending, screen_issue, error) || name.empty() ||
            !entries_.emplace(name, Entry{id, mode, cls, supported, version, alpha_ref, blending, screen_issue}).second)
        {
            if (error.empty()) error = "duplicate or empty shaders.xr blender name: " + name;
            error = "shaders.xr blender id=" + std::to_string(id) +
                " class='" + cls + "' version=" + std::to_string(version) +
                " shader='" + name + "': " + error;
            record->close();
            clear();
            return false;
        }
        Msg("[renderer-vulkan] material.definition file=shaders.xr id=%u name='%s' class='%s' version=%u mode=%s alpha-ref=%d blending=%d supported=%d",
            id, name.c_str(), cls.c_str(), version,
            mode == SurfaceMode::Transparent ? "transparent" : mode == SurfaceMode::AlphaTest ? "cutout" : "opaque",
            alpha_ref, blending, supported ? 1 : 0);
    }
    if (entries_.empty()) { error = "shaders.xr contains no blender definitions"; return false; }
    error.clear();
    return true;
}

bool ShaderMaterialLibrary::resolve(const std::string& shader, SurfaceMode& mode,
    std::string& error, int* alpha_ref, int* blend_mode,
    bool particle_pipeline, bool screen_pipeline) const
{
    const auto it = entries_.find(lower(shader));
    if (it == entries_.end())
    { error = "shaders.xr shader='" + shader + "' not found"; return false; }
    const auto& material = it->second;
    const std::string context = "shaders.xr blender id=" + std::to_string(material.id) +
        " class='" + material.class_name + "' version=" + std::to_string(material.version) +
        " shader='" + shader + "'";
    if (!material.supported)
    { error = "unsupported " + context; return false; }
    if (screen_pipeline && material.class_name != "S_SET   ")
    { error = "unsupported screen class in " + context; return false; }
    if (material.class_name == "S_SET   " && !material.screen_issue.empty())
    {
        error = "unsupported screen property='" + material.screen_issue + "' in " + context;
        return false;
    }
    // The scene pipeline currently implements only source-alpha blending.
    // ADD/MUL and legacy multi-render-target modes must not silently render as BLEND.
    if ((material.class_name == "S_SET   " || material.class_name == "PARTICLE") &&
        material.blending != 0 && material.blending != 1 &&
        material.blending != 2 && material.blending != 3 &&
        material.blending != 4 && material.blending != 5 &&
        !(screen_pipeline && material.class_name == "S_SET   " &&
          material.blending >= 6 && material.blending <= 9))
    { error = "unsupported blending=" + std::to_string(material.blending) + " in " + context; return false; }
    // Screen-set blending outside particles still needs its own pipeline.
    // The particle path has a pipeline for each of these modes.
    if (material.class_name == "S_SET   " && !particle_pipeline && !screen_pipeline &&
        material.blending >= 2)
    { error = "unsupported blending=" + std::to_string(material.blending) + " in " + context; return false; }
    if (material.class_name == "PARTICLE" && !particle_pipeline &&
        (material.blending == 3 || material.blending == 4))
    { error = "unsupported blending=" + std::to_string(material.blending) + " in " + context; return false; }
    mode = it->second.mode;
    if (alpha_ref) *alpha_ref = it->second.alpha_ref >= 0 ?
        std::clamp(it->second.alpha_ref, 0, 255) : 128;
    if (blend_mode) *blend_mode = material.blending;
    error.clear();
    return true;
}

bool ShaderMaterialLibrary::contains(const std::string& shader) const
{
    return entries_.find(lower(shader)) != entries_.end();
}

}
