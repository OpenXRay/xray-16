#include "xrEngine/stdafx.h"
#include "ShaderMaterialLibrary.h"
#include "xrCore/stream_reader.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <cmath>

namespace xray::render::vulkan
{
std::array<std::array<float, 4>, 3> TextureMatrix::evaluate(float time, const float* view) const
{
    std::array<std::array<float, 4>, 3> rows{{{{1, 0, 0, 0}}, {{0, 1, 0, 0}}, {{0, 0, 1, 0}}}};
    if (mode == 1)
    {
        auto wave = waves;
        const float angle = (tcm & 2) ? wave[2].Calculate(time) * time : 0.f;
        const float su = (tcm & 1) ? wave[0].Calculate(time) : 1.f;
        const float sv = (tcm & 1) ? wave[1].Calculate(time) : 1.f;
        const float c = std::cos(angle), s = std::sin(angle);
        rows[0][0] = su * c; rows[0][1] = -su * s;
        rows[1][0] = sv * s; rows[1][1] = sv * c;
        const float scroll_u = (tcm & 4) ? wave[3].Calculate(time) * time * su : 0.f;
        const float scroll_v = (tcm & 4) ? wave[4].Calculate(time) * time * sv : 0.f;
        rows[0][3] = .5f - .5f * (rows[0][0] + rows[0][1]) + scroll_u;
        rows[1][3] = .5f - .5f * (rows[1][0] + rows[1][1]) + scroll_v;
    }
    else if (mode == 2 && view)
    {
        rows[0] = {{.5f * view[0], .5f * view[4], .5f * view[8], .5f}};
        rows[1] = {{-.5f * view[1], -.5f * view[5], -.5f * view[9], .5f}};
    }
    // Reflection directions are already in world space in the Vulkan shaders.
    // The legacy modeC_refl inverse-view therefore has no further effect.
    return rows;
}
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
    uint16_t& version, int& alpha_ref, int& blending, uint32_t& screen_flags,
    TerrainMaterialTextures& terrain, std::vector<BlenderProperty>& properties, std::string& error)
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
    screen_flags = 0;
    terrain = {};
    properties.clear();
    std::string scope;
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
        if (type == 0) scope = property;
        properties.push_back({scope, property, type,
            std::vector<uint8_t>(bytes + start, bytes + start + length)});
        if (cls_name == "BmmDold " || cls_name == "BmmD    " || cls_name == "LmBmmD  ")
        {
            constexpr const char* channels[]{"R2-R", "R2-G", "R2-B", "R2-A"};
            for (size_t channel = 0; channel < 4; ++channel)
                if (property == channels[channel])
                {
                    if (type != 3 || !std::memchr(bytes + start, 0, length))
                    { error = "invalid terrain texture property='" + property + "'"; return false; }
                    terrain.detail[channel] = lower(reinterpret_cast<const char*>(bytes + start));
                }
        }
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
        if (type == 6)
        {
            const bool enabled = le32(bytes + start) != 0;
            if (property == "Texture clamp" && !enabled) screen_flags |= 1;
            if (property == "Z-test" && enabled) screen_flags |= 2;
            if (property == "Z-write" && enabled) screen_flags |= 4;
            if (property == "Lighting" && enabled) screen_flags |= 8;
            if (property == "Fog" && enabled) screen_flags |= 16;
        }
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
    if (cls == "BmmDold " || cls == "BmmD    " || cls == "LmBmmD  ")
    {
        const bool any = std::any_of(terrain.detail.begin(), terrain.detail.end(),
            [](const std::string& texture) { return !texture.empty() && texture != "$null"; });
        const bool all = std::all_of(terrain.detail.begin(), terrain.detail.end(),
            [](const std::string& texture) { return !texture.empty() && texture != "$null"; });
        if (any && !all)
        { error = "incomplete terrain R2-R/G/B/A texture set in '" + name + "'"; return false; }
        terrain.enabled = all;
    }
    if ((cls == "PARTICLE" || cls == "S_SET   ") && !has_blending)
    { error = "missing Blending property in '" + name + "'"; return false; }
    // Deferred D_TREE alpha blending selects the base_aref shader and writes
    // depth. Treating its leaves as forward transparency makes the canopy
    // see-through and prevents it from occluding distant lights.
    // CBlender_deffer_model only uses forward alpha blending when AREF < 16;
    // higher thresholds use the depth-writing deferred base_aref pass.
    const bool model_cutout = cls == "MODEL   " && blend && alpha_ref >= 16;
    if ((cls == "PARTICLE" && blending != 0) || strict || (cls == "S_SET   " && blending != 0) ||
        ((cls == "LM_AREF " || cls == "V_AREF  " || cls == "MODELEbB" || cls == "LmEbB   ") && blend) ||
        (cls == "MODEL   " && blend && !model_cutout))
        mode = SurfaceMode::Transparent;
    else if ((cls == "PARTICLE" && blending == 0) || cls == "LM_AREF " || cls == "V_AREF  " || cls == "D_TREE  " ||
        (cls == "D_STILL " && blend) || model_cutout)
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
    std::unordered_map<std::string, std::array<WaveForm, 4>> constants;
    std::unique_ptr<IReader, void(*)(IReader*)> constants_chunk(file.open_chunk(0),
        [](IReader* p) { if (p) p->close(); });
    if (constants_chunk)
    {
        Cursor cursor{static_cast<const uint8_t*>(constants_chunk->pointer()), constants_chunk->length(), 0};
        while (cursor.offset < cursor.size)
        {
            std::string name;
            if (!cursor.string(name) || name.empty() || cursor.size - cursor.offset < sizeof(WaveForm) * 4)
            { error = "shaders.xr constant table is truncated"; return false; }
            std::array<WaveForm, 4> waveforms;
            std::memcpy(waveforms.data(), cursor.bytes + cursor.offset, sizeof(WaveForm) * 4);
            cursor.offset += sizeof(WaveForm) * 4;
            for (const WaveForm& wave : waveforms)
            {
                if (wave.F > WaveForm::fINVSAWTOOTH ||
                    std::any_of(std::begin(wave.arg), std::end(wave.arg),
                        [](float value) { return !std::isfinite(value); }))
                { error = "shaders.xr constant='" + name + "' has invalid waveform"; return false; }
            }
            if (!constants.emplace(lower(name), waveforms).second)
            { error = "shaders.xr duplicate constant='" + name + "'"; return false; }
        }
    }
    std::unordered_map<std::string, TextureMatrix> matrices;
    std::unique_ptr<IReader, void(*)(IReader*)> matrices_chunk(file.open_chunk(1),
        [](IReader* p) { if (p) p->close(); });
    if (matrices_chunk)
    {
        Cursor cursor{static_cast<const uint8_t*>(matrices_chunk->pointer()), matrices_chunk->length(), 0};
        while (cursor.offset < cursor.size)
        {
            std::string name;
            constexpr size_t matrix_bytes = 8 + 5 * sizeof(WaveForm);
            if (!cursor.string(name) || name.empty() || cursor.size - cursor.offset < matrix_bytes)
            { error = "shaders.xr matrix table is truncated"; return false; }
            TextureMatrix matrix;
            matrix.mode = le32(cursor.bytes + cursor.offset);
            matrix.tcm = le32(cursor.bytes + cursor.offset + 4);
            std::memcpy(matrix.waves.data(), cursor.bytes + cursor.offset + 8, 5 * sizeof(WaveForm));
            cursor.offset += matrix_bytes;
            if (matrix.mode > 4 || (matrix.tcm & ~7u))
            { error = "shaders.xr matrix='" + name + "' has invalid mode/flags"; return false; }
            for (const auto& wave : matrix.waves)
                if (wave.F > WaveForm::fINVSAWTOOTH ||
                    std::any_of(std::begin(wave.arg), std::end(wave.arg),
                        [](float value) { return !std::isfinite(value); }))
                { error = "shaders.xr matrix='" + name + "' has invalid waveform"; return false; }
            if (!matrices.emplace(lower(name), matrix).second)
            { error = "shaders.xr duplicate matrix='" + name + "'"; return false; }
        }
    }
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
        uint32_t screen_flags = 0;
        TerrainMaterialTextures terrain;
        std::vector<BlenderProperty> properties;
        if (!parse_record(static_cast<const uint8_t*>(record->pointer()), record->length(),
                name, mode, supported, cls, version, alpha_ref, blending, screen_flags,
                terrain, properties, error) || name.empty() ||
            ![&]() {
                MaterialDefinition entry;
                entry.id = id; entry.shader = name; entry.class_name = cls;
                entry.version = version; entry.mode = mode; entry.pipeline_supported = supported;
                entry.alpha_ref = alpha_ref; entry.blending = blending;
                entry.screen_flags = screen_flags; entry.terrain = terrain;
                entry.properties = std::move(properties);
                return entries_.emplace(name, std::move(entry)).second;
            }())
        {
            if (error.empty()) error = "duplicate or empty shaders.xr blender name: " + name;
            error = "shaders.xr blender id=" + std::to_string(id) +
                " class='" + cls + "' version=" + std::to_string(version) +
                " shader='" + name + "': " + error;
            record->close();
            clear();
            return false;
        }
        auto& definition = entries_.at(name);
        for (const BlenderProperty& property : definition.properties)
            if (property.scope == "General" && property.name == "Priority" &&
                property.type == 4 && property.data.size() == 12)
                definition.priority = static_cast<uint8_t>(std::min(le32(property.data.data()), 255u));
        for (const BlenderProperty& property : definition.properties)
            if (cls == "D_TREE  " && property.scope == "Base Texture" &&
                property.name == "Object LOD" && property.type == 6 && property.data.size() == 4)
                definition.object_lod = le32(property.data.data()) != 0;
        for (const BlenderProperty& property : definition.properties)
            if (property.scope == "Base Texture" && property.name == "Tessellation" &&
                (property.type == 7 || property.type == 8) && property.data.size() >= 8 &&
                le32(property.data.data()) <= 3)
                definition.tessellation = static_cast<uint8_t>(le32(property.data.data()));
        const bool strict_sorting = std::any_of(definition.properties.begin(), definition.properties.end(),
            [](const BlenderProperty& property)
            { return property.name == "Strict sorting" && property.type == 6 &&
                property.data.size() == 4 && le32(property.data.data()) != 0; });
        definition.pass.strict_sorting = strict_sorting;
        definition.pass.depth_write = mode != SurfaceMode::Transparent;
        if (cls == "S_SET   ")
        {
            definition.pass.depth_test = (screen_flags & 2u) != 0;
            definition.pass.depth_write = (screen_flags & 4u) != 0;
            definition.pass.lighting = (screen_flags & 8u) != 0;
            definition.pass.fog = (screen_flags & 16u) != 0;
            definition.pass.clamp = (screen_flags & 1u) == 0;
        }
        else if (cls == "PARTICLE")
        {
            definition.pass.depth_write = blending == 0;
            definition.pass.clamp = (screen_flags & 1u) == 0;
        }
        // The game's water S_SET variant declares depth-tested, depth-writing,
        // lit and sorted transparent geometry. Its role comes from serialized
        // state, so a mod can rename the shader and texture without rerouting.
        if (cls == "S_SET   " && blending == 0 && strict_sorting &&
            (screen_flags & 14u) == 14u)
            definition.role = MaterialDefinition::Role::Water;
        if (cls == "LmEbB   " || cls == "MODELEbB" || cls == "LaEmB   ")
        {
            for (const BlenderProperty& property : definition.properties)
            {
                if (property.scope != "Environment map" ||
                    (property.name != "Name" && property.name != "Transform" && property.name != "Constant")) continue;
                const uint32_t expected_type = property.name == "Transform" ? 1u :
                    property.name == "Constant" ? 2u : 3u;
                if (property.type != expected_type ||
                    property.data.empty() || !std::memchr(property.data.data(), 0, property.data.size()))
                {
                    definition.pipeline_supported = false;
                    definition.unsupported_reason = "Environment map / " + property.name + " (invalid property)";
                    break;
                }
                const std::string value = lower(reinterpret_cast<const char*>(property.data.data()));
                if (property.name == "Name") definition.environment_texture = value;
                if (property.name == "Transform") definition.environment_transform = value;
                if (property.name == "Constant") definition.environment_constant = value;
            }
            const bool uses_environment = cls != "MODELEbB" || mode == SurfaceMode::Transparent;
            if (definition.pipeline_supported && uses_environment &&
                (definition.environment_texture.empty() || definition.environment_texture == "$null"))
            {
                definition.pipeline_supported = false;
                definition.unsupported_reason = "Environment map / Name (required texture)";
            }
            if (definition.pipeline_supported && cls == "LaEmB   " &&
                !definition.environment_constant.empty() && definition.environment_constant != "$null")
            {
                const auto constant = constants.find(definition.environment_constant);
                if (constant == constants.end())
                {
                    definition.pipeline_supported = false;
                    definition.unsupported_reason = "Environment map / Constant '" +
                        definition.environment_constant + "' (missing shaders.xr constant)";
                }
                else
                {
                    definition.constant_waveforms = constant->second;
                    definition.has_environment_constant = true;
                }
            }
        }
        // BmmD Save() serializes Name even for the v3 four-channel terrain
        // path. The original impl shader uses R2-R/G/B/A for that path and
        // ignores Name as a diffuse detail sampler. Preserve the slot for
        // inspection; only the one-channel variant consumes it.
        if (cls == "BmmDold " || cls == "BmmD    " || cls == "LmBmmD  ")
        {
            if (!terrain.enabled)
            {
                definition.pipeline_supported = false;
                definition.unsupported_reason = "Detail map / R2-R/G/B/A (single detail variant has no Vulkan pipeline)";
            }
            for (const BlenderProperty& property : definition.properties)
            {
                if (property.scope != "Detail map" || property.name != "Name") continue;
                if (property.type != 3 || property.data.empty() ||
                    !std::memchr(property.data.data(), 0, property.data.size()))
                {
                    definition.pipeline_supported = false;
                    definition.unsupported_reason = "Detail map / Name (invalid texture slot)";
                    break;
                }
                const std::string detail(reinterpret_cast<const char*>(property.data.data()));
                if (!detail.empty() && lower(detail) != "$null")
                {
                    definition.detail_texture = lower(detail);
                    if (!terrain.enabled && supported)
                    {
                        definition.pipeline_supported = true;
                        definition.unsupported_reason.clear();
                    }
                }
                break;
            }
        }
        // Preserve every serialized property, but only advertise a renderable
        // material when all non-default properties have an implemented path.
        // Modded shaders.xr can add properties without changing the class ID.
        for (const BlenderProperty& property : definition.properties)
        {
            if (!definition.pipeline_supported) break;
            const auto& name = property.name;
            const auto& scope = property.scope;
            if (property.type == 0)
            {
                const bool detail_class = cls == "BmmDold " || cls == "BmmD    " || cls == "LmBmmD  ";
                const bool environment_class = cls == "LmEbB   " || cls == "MODELEbB" || cls == "LaEmB   ";
                if (name != "General" && name != "Base Texture" &&
                    !(name == "Detail map" && detail_class) &&
                    !(name == "Environment map" && environment_class))
                {
                    definition.pipeline_supported = false;
                    definition.unsupported_reason = name + " (scope does not belong to blender class)";
                }
                continue;
            }
            const bool common = (scope == "General" &&
                (name == "Priority" || name == "Strict sorting")) ||
                (scope == "Base Texture" && (name == "Name" || name == "Transform"));
            const bool detail_class = cls == "BmmDold " || cls == "BmmD    " || cls == "LmBmmD  ";
            const bool environment_class = cls == "LmEbB   " || cls == "MODELEbB" || cls == "LaEmB   ";
            const bool screen_class = cls == "S_SET   ";
            const bool particle_class = cls == "PARTICLE";
            // Properties follow the Save() schema of the concrete blender,
            // not just a shared spelling. An accepted but unused property
            // would falsely advertise a material as renderable.
            const bool known = common ||
                (scope == "Base Texture" && (
                    (name == "Alpha ref" && (cls == "LM_AREF " || cls == "V_AREF  " ||
                        (cls == "MODEL   " && version >= 1) || screen_class || particle_class)) ||
                    (name == "Alpha-blend" && ((cls == "LM_AREF " && version >= 1) ||
                        (cls == "V_AREF  " && version >= 1) ||
                        cls == "D_TREE  " || cls == "D_STILL ")) ||
                    (name == "Use alpha-channel" && cls == "MODEL   " && version >= 1) ||
                    (name == "Object LOD" && cls == "D_TREE  " && version >= 1) ||
                    (name == "Tessellation" && ((cls == "LM      " || cls == "V       ") &&
                        version >= 1 || cls == "MODEL   " && version >= 2)) ||
                    (name == "Blending" && (screen_class || particle_class)) ||
                    (name == "Texture clamp" && (screen_class || particle_class)) ||
                    ((name == "Z-test" || name == "Z-write" || name == "Lighting" ||
                        name == "Fog") && screen_class))) ||
                (scope == "Detail map" && detail_class &&
                    (name == "Name" || name == "Transform" ||
                     (version >= 3 && (name == "R2-R" || name == "R2-G" ||
                        name == "R2-B" || name == "R2-A")))) ||
                (scope == "Environment map" && environment_class &&
                    (name == "Name" || name == "Transform" ||
                     (name == "Constant" && cls == "LaEmB   ") ||
                     (name == "Alpha-Blend" && cls != "LaEmB   " && version >= 1)));
            const std::string path = scope + " / " + name;
            const auto reject = [&](const char* reason)
            {
                definition.pipeline_supported = false;
                definition.unsupported_reason = path + " (" + reason + ")";
            };
            if (!known) { reject("unknown material semantics"); break; }
            const bool boolean = name == "Strict sorting" || name == "Alpha-blend" ||
                name == "Alpha-Blend" || name == "Use alpha-channel" ||
                name == "Object LOD" || name == "Texture clamp" ||
                name == "Z-test" || name == "Z-write" || name == "Lighting" || name == "Fog";
            const bool string_slot = name == "Name" || name == "Transform" ||
                name == "Constant" || name == "R2-R" || name == "R2-G" ||
                name == "R2-B" || name == "R2-A";
            if ((boolean && (property.type != 6 || property.data.size() != 4)) ||
                (name == "Priority" && (property.type != 4 || property.data.size() != 12)) ||
                (name == "Alpha ref" && (property.type != 4 || property.data.size() != 12)) ||
                ((name == "Tessellation" || name == "Blending") &&
                    property.type != 7 && property.type != 8) ||
                (string_slot && ((name == "Transform" ? property.type != 1 :
                    name == "Constant" ? property.type != 2 : property.type != 3) ||
                    property.data.size() != 64 ||
                    !std::memchr(property.data.data(), 0, property.data.size()))))
            { reject("invalid property type or value"); break; }
            if (name == "Priority" && property.type == 4 && property.data.size() == 12 &&
                (le32(property.data.data()) > 3 ||
                 le32(property.data.data()) < le32(property.data.data() + 4) ||
                 le32(property.data.data()) > le32(property.data.data() + 8)))
                reject("invalid render priority (expected 0..3 within saved bounds)");
            if (name == "Tessellation" && (property.type == 7 || property.type == 8))
            {
                const uint32_t selected = le32(property.data.data());
                const uint32_t count = le32(property.data.data() + 4);
                const size_t first = property.type == 7 ? 8 : 12;
                const size_t stride = property.type == 7 ? 68 : 8;
                bool listed = count == 0;
                for (uint32_t item = 0; item < count; ++item)
                    listed |= le32(property.data.data() + first + size_t(item) * stride) == selected;
                if (selected > 3 || !listed)
                    reject("invalid Tessellation token (expected NO_TESS/PN/HM/PN+HM)");
                // The selected PN/HM mode is applied when level or OGF
                // geometry is uploaded.
            }
            // The four-channel BmmD impl pass does not bind oT2_xform: it
            // samples R2-R/G/B/A with its terrain UV. The serialized detail
            // matrix remains relevant to the one-channel detail variant.
            // The programmable S_SET blender binds C.L_textures[0] and
            // ignores its saved Base Texture Name/Transform. Fixed-function
            // S_SET uses those fields, but Vulkan never runs that path.
            if (name == "Transform" && property.type == 1 && property.data[0])
            {
                const std::string matrix_name = lower(reinterpret_cast<const char*>(property.data.data()));
                const bool consumed = (scope == "Base Texture" && cls != "S_SET   ") ||
                    (scope == "Detail map" && !terrain.enabled) ||
                    (scope == "Environment map" &&
                        (cls != "MODELEbB" || mode == SurfaceMode::Transparent));
                if (consumed && matrix_name != "$null")
                {
                    const auto found = matrices.find(matrix_name);
                    if (found == matrices.end()) reject("missing shaders.xr matrix");
                    else
                    {
                        const size_t slot = scope == "Base Texture" ? 0 : scope == "Detail map" ? 1 : 2;
                        definition.matrices.values[slot] = found->second;
                        definition.matrices.enabled[slot] = true;
                    }
                }
            }
            if (name == "Name" && scope == "Base Texture" && cls != "S_SET   " &&
                property.type == 3 && property.data[0] &&
                std::strcmp(reinterpret_cast<const char*>(property.data.data()), "$base0") != 0)
                reject("base texture selector is not implemented");
        }
        Msg("[renderer-vulkan] material.definition file=shaders.xr id=%u name='%s' class='%s' version=%u mode=%s priority=%u alpha-ref=%d blending=%d flags=0x%x role=%s terrain=%d detail='%s' environment='%s' transform='%s' constant='%s' tessellation=%u passes=%u properties=%zu supported=%d reason='%s'",
            id, name.c_str(), cls.c_str(), version,
            mode == SurfaceMode::Transparent ? "transparent" : mode == SurfaceMode::AlphaTest ? "cutout" : "opaque",
            definition.priority, alpha_ref, blending, definition.screen_flags,
            definition.role == MaterialDefinition::Role::Water ? "water" : "standard",
            terrain.enabled ? 1 : 0, definition.detail_texture.c_str(),
            definition.environment_texture.c_str(), definition.environment_transform.c_str(),
            definition.environment_constant.c_str(), definition.tessellation,
            cls == "LaEmB   " ? 2u : 1u,
            definition.properties.size(), definition.pipeline_supported ? 1 : 0,
            definition.unsupported_reason.c_str());
    }
    if (entries_.empty()) { error = "shaders.xr contains no blender definitions"; return false; }
    error.clear();
    return true;
}

bool ShaderMaterialLibrary::resolve(const std::string& shader, SurfaceMode& mode,
    std::string& error, int* alpha_ref, int* blend_mode,
    bool particle_pipeline, bool screen_pipeline, bool level_pipeline,
    bool water_pipeline, uint32_t* material_flags, TerrainMaterialTextures* terrain,
    MaterialDefinition* definition, bool model_pipeline) const
{
    const auto it = entries_.find(lower(shader));
    if (it == entries_.end())
    { error = "shaders.xr shader='" + shader + "' not found"; return false; }
    const auto& material = it->second;
    const std::string context = "shaders.xr blender id=" + std::to_string(material.id) +
        " class='" + material.class_name + "' version=" + std::to_string(material.version) +
        " shader='" + shader + "'";
    if (!material.pipeline_supported)
    { error = "unsupported " + context + (material.unsupported_reason.empty() ? "" :
        " property='" + material.unsupported_reason + "'"); return false; }
    if (material.priority != 1 &&
        !(level_pipeline && !model_pipeline && material.priority >= 2 &&
          material.priority <= 3 &&
          (material.mode == SurfaceMode::Transparent ||
           material.class_name == "V       " && material.mode == SurfaceMode::Opaque)) &&
        !(particle_pipeline && material.class_name == "PARTICLE" &&
          material.priority >= 2 && material.priority <= 3))
    {
        error = "unsupported General / Priority=" + std::to_string(material.priority) +
            " in " + context + " for this render consumer";
        return false;
    }
    if (screen_pipeline && material.class_name != "S_SET   ")
    { error = "unsupported screen class in " + context; return false; }
    // World decals and tracers use S_SET too. Their Z-test=1/Z-write=0
    // matches the forward transparent pass, whereas the UI pass has no
    // depth attachment. Keep rejecting properties the selected pass cannot
    // honor rather than treating every S_SET as an overlay.
    const bool world_set = level_pipeline && material.class_name == "S_SET   " &&
        material.blending == 0 && material.role != MaterialDefinition::Role::Water;
    const bool particle_screen_set = particle_pipeline && material.class_name == "S_SET   ";
    const bool scene_depth_test = !screen_pipeline &&
        (material.mode == SurfaceMode::Transparent || world_set || particle_screen_set);
    // UI has no depth attachment, but wrap addressing is a valid sampler state.
    const uint32_t supported_flags = screen_pipeline ? 1u :
        1u | (scene_depth_test ? 2u : 0u) |
        (level_pipeline && !model_pipeline && material.blending == 1 ? 16u : 0u) |
        ((water_pipeline || (level_pipeline && material.role == MaterialDefinition::Role::Water)) ? 12u : 0u) |
        (world_set || (particle_screen_set && material.blending == 0) ? 4u : 0u);
    const uint32_t unsupported_flags = material.screen_flags & ~supported_flags;
    if (material.class_name == "S_SET   " && unsupported_flags)
    {
        const char* property = unsupported_flags & 1 ? "Texture clamp" : unsupported_flags & 2 ? "Z-test" :
            unsupported_flags & 4 ? "Z-write" : unsupported_flags & 8 ? "Lighting" : "Fog";
        error = "unsupported screen property='" + std::string(property) + "' in " + context;
        return false;
    }
    if (material.class_name == "S_SET   " && level_pipeline &&
        !(material.screen_flags & 2u))
    {
        error = "unsupported world property='Z-test=0' in " + context;
        return false;
    }
    if (particle_screen_set && !(material.screen_flags & 2u))
    {
        error = "unsupported particle property='Z-test=0' in " + context;
        return false;
    }
    if (particle_screen_set && ((material.screen_flags & 4u) != 0) != (material.blending == 0))
    {
        error = "unsupported particle property='Z-write' for S_SET Blending=" +
            std::to_string(material.blending) + " in " + context;
        return false;
    }
    if (material.class_name == "S_SET   " && level_pipeline &&
        material.mode != SurfaceMode::Transparent &&
        !(material.screen_flags & 4u))
    {
        error = "unsupported world property='Z-write=0' in " + context;
        return false;
    }
    if (world_set && !material.pass.depth_write)
    {
        error = "unsupported world property='Z-write=0' with S_SET Blending=0 in " + context;
        return false;
    }
    // Scene geometry has explicit blend variants for additive, multiply,
    // multiply-2x, alpha-add and wallmark multiply-2x. Other modes must not
    // silently render as source-alpha blending.
    if ((material.class_name == "S_SET   " || material.class_name == "PARTICLE") &&
        material.blending != 0 && material.blending != 1 &&
        material.blending != 2 && material.blending != 3 &&
        material.blending != 4 && material.blending != 5 &&
        !(material.class_name == "S_SET   " && !screen_pipeline && !particle_pipeline &&
          (material.blending == 6 || (level_pipeline && material.blending >= 7 && material.blending <= 9))) &&
        !(screen_pipeline && material.class_name == "S_SET   " &&
          material.blending >= 6 && material.blending <= 9))
    { error = "unsupported blending=" + std::to_string(material.blending) + " in " + context; return false; }
    // Screen-set blending outside particles still needs its own pipeline.
    // The particle path has a pipeline for each of these modes.
    if (material.class_name == "S_SET   " && !particle_pipeline && !screen_pipeline &&
        material.blending >= 2 && material.blending != 6 &&
        !(level_pipeline && ((material.blending >= 2 && material.blending <= 5) ||
            (material.blending >= 7 && material.blending <= 9))))
    { error = "unsupported blending=" + std::to_string(material.blending) + " in " + context; return false; }
    if (material.class_name == "PARTICLE" && !particle_pipeline &&
        (material.blending == 3 || material.blending == 4))
    { error = "unsupported blending=" + std::to_string(material.blending) + " in " + context; return false; }
    // Unlit world SET writes depth in the forward attachment after combine;
    // its output must not be lit again as an ordinary G-buffer surface.
    mode = world_set ? SurfaceMode::Transparent : it->second.mode;
    // Screen SET's 2R replacement enables alpha test with reference zero in
    // Compile(), irrespective of its serialized Alpha ref property.
    if (alpha_ref) *alpha_ref = material.class_name == "PARTICLE" && material.blending == 0 ? 200 :
        material.class_name == "S_SET   " && material.blending == 7 ? 0 :
        it->second.alpha_ref >= 0 ?
        std::clamp(it->second.alpha_ref, 0, 255) : 128;
    if (blend_mode) *blend_mode = material.blending;
    if (material_flags) *material_flags = material.screen_flags;
    if (terrain) *terrain = material.terrain;
    if (definition) *definition = material;
    error.clear();
    return true;
}

bool ShaderMaterialLibrary::contains(const std::string& shader) const
{
    return entries_.find(lower(shader)) != entries_.end();
}

const MaterialDefinition* ShaderMaterialLibrary::find(const std::string& shader) const
{
    const auto it = entries_.find(lower(shader));
    return it == entries_.end() ? nullptr : &it->second;
}

}
