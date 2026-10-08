#include "xrEngine/stdafx.h"
#include "EngineLevelModels.h"

#include "Common/LevelStructure.hpp"
#include "xrCore/FMesh.hpp"
#include "xrCore/stream_reader.h"
#include "xrCore/lzhuf.h"
#include "xrEngine/defines.h"

#include <algorithm>
#include <memory>
#include <unordered_set>

namespace xray::render::vulkan
{
bool load_engine_detail_assets(DetailAssets& result, std::string& error)
{
    if (!FS.exist("$level$", "level.details"))
    {
        result = {};
        error.clear();
        return true;
    }
    IReader* file = FS.r_open("$level$", "level.details");
    if (!file)
    {
        error = "cannot open level.details";
        return false;
    }
    IReader* header = file->open_chunk(0);
    IReader* models = file->open_chunk(1);
    IReader* slots = file->open_chunk(2);
    bool valid = header && models && slots && header->length() == 24;
    std::vector<LevelBytes> objects;
    std::vector<IReader*> object_readers;
    if (valid)
    {
        const uint8_t* h = static_cast<const uint8_t*>(header->pointer());
        const uint32_t count = uint32_t(h[4]) | uint32_t(h[5]) << 8 | uint32_t(h[6]) << 16 | uint32_t(h[7]) << 24;
        valid = count <= 64;
        for (uint32_t id = 0; valid && id < count; ++id)
        {
            IReader* object = models->open_chunk(id);
            if (!object) { valid = false; break; }
            // The decoder needs stable payloads until every object has been
            // checked; retain the readers rather than borrowing closed chunks.
            objects.push_back({static_cast<const uint8_t*>(object->pointer()), object->length()});
            object_readers.push_back(object);
        }
    }
    if (!valid)
        error = "invalid level.details chunk table";
    else
        valid = decode_detail_assets({static_cast<const uint8_t*>(header->pointer()), header->length()},
            objects, {static_cast<const uint8_t*>(slots->pointer()), slots->length()}, result, error);
    for (IReader* reader : object_readers) reader->close();
    if (header) header->close();
    if (models) models->close();
    if (slots) slots->close();
    FS.r_close(file);
    if (!valid) error = "level.details: " + error;
    return valid;
}
namespace
{
void append_chunk(std::vector<uint8_t> &bytes, uint32_t id, const uint8_t *data, size_t size);
bool normalize_chunks(IReader &reader, bool table, std::vector<uint8_t> &output, std::string &error, unsigned depth)
{
    if (depth > 64 || reader.length() > 64 * 1024 * 1024)
    {
        error = "OGF chunk nesting or size limit exceeded";
        return false;
    }
    reader.rewind();
    std::unordered_set<u32> seen;
    std::vector<uint8_t> normalized;
    while (!reader.eof())
    {
        if (reader.elapsed() < 8)
        {
            error = "truncated OGF chunk header";
            return false;
        }
        const u32 encoded = reader.r_u32(), size = reader.r_u32();
        const u32 id = encoded & ~CFS_CompressMark;
        const size_t end = reader.tell() + size;
        if (size > reader.elapsed() || !seen.insert(id).second || ((encoded & CFS_CompressMark) && size < 4))
        {
            error = "invalid OGF chunk id=" + std::to_string(id);
            return false;
        }
        if (!size)
        {
            append_chunk(normalized, id, nullptr, 0);
            continue;
        }
        std::vector<uint8_t> unpacked;
        auto *body_data = static_cast<uint8_t *>(reader.pointer());
        size_t body_size = size;
        if (encoded & CFS_CompressMark)
        {
            const uint32_t decoded_size =
                uint32_t(body_data[0]) | (uint32_t(body_data[1]) << 8) | (uint32_t(body_data[2]) << 16) | (uint32_t(body_data[3]) << 24);
            if (!decoded_size || decoded_size > 64 * 1024 * 1024)
            {
                error = "invalid compressed OGF size id=" + std::to_string(id);
                return false;
            }
            uint8_t *decoded = nullptr;
            size_t length = 0;
            // Use the engine codec, but check its result before constructing
            // a reader. IReader::open_chunk assumes decompression succeeds.
            const bool valid = _decompressLZ(&decoded, &length, body_data, size, 64 * 1024 * 1024);
            if (!valid || !decoded || length != decoded_size)
            {
                xr_free(decoded);
                error = "cannot decompress OGF chunk id=" + std::to_string(id);
                return false;
            }
            unpacked.assign(decoded, decoded + length);
            xr_free(decoded);
            body_data = unpacked.data();
            body_size = unpacked.size();
        }
        IReader body(body_data, body_size);
        std::vector<uint8_t> nested;
        const bool recursive = table || id == OGF_CHILDREN || id == OGF_FASTPATH;
        const bool loaded = !recursive || normalize_chunks(body, !table && id == OGF_CHILDREN, nested, error, depth + 1);
        if (loaded)
            append_chunk(normalized, id, recursive ? nested.data() : body_data, recursive ? nested.size() : body_size);
        if (!loaded)
        {
            error = "chunk id=" + std::to_string(id) + " / " + error;
            return false;
        }
        reader.seek(end);
    }
    output = std::move(normalized);
    return true;
}
LevelGameProfile game_profile()
{
    return ShadowOfChernobylMode ? LevelGameProfile::SoC : ClearSkyMode ? LevelGameProfile::CS : LevelGameProfile::CoP;
}
} // namespace

bool normalize_engine_visual_chunks(IReader &reader, bool table, std::vector<uint8_t> &result, std::string &error)
{
    const size_t position = reader.tell();
    const bool loaded = normalize_chunks(reader, table, result, error, 0);
    reader.seek(position);
    if (loaded)
        error.clear();
    return loaded;
}

bool load_engine_model_reader(IReader &reader, const char *name, VisualRecord &result, std::string &error)
{
    std::vector<uint8_t> bytes;
    if (normalize_engine_visual_chunks(reader, false, bytes, error) && parse_ogf_visual({bytes.data(), bytes.size()}, result, error))
        return true;
    error = std::string(name && *name ? name : "<reader>.ogf") + " / OGF decode: " + error;
    return false;
}

bool load_engine_visual_catalog(IReader &level, std::vector<VisualRecord> &result, std::string &error)
{
    IReader *visuals = level.open_chunk(fsL_VISUALS);
    if (!visuals)
    {
        error = "level has no OGF visual table";
        return false;
    }
    std::vector<uint8_t> bytes;
    const bool loaded = normalize_engine_visual_chunks(*visuals, true, bytes, error) && parse_level_visuals({bytes.data(), bytes.size()}, result, error);
    visuals->close();
    return loaded;
}

bool load_engine_model_visual(const char* name, VisualRecord& result, std::string& error)
{
    if (!name || !*name)
    {
        error = "model name is empty";
        return false;
    }
    string_path filename;
    if (strext(name)) xr_strcpy(filename, name);
    else strconcat(sizeof(filename), filename, name, ".ogf");
    string_path path;
    if (!FS.exist(filename) && !FS.exist(path, "$level$", filename) &&
        !FS.exist(path, "$game_meshes$", filename))
    {
        error = std::string("model not found: ") + filename;
        return false;
    }
    IReader* reader = FS.r_open(FS.exist(filename) ? filename : path);
    if (!reader)
    {
        error = std::string("model cannot be opened: ") + path;
        return false;
    }
    const bool loaded = load_engine_model_reader(*reader, filename, result, error);
    FS.r_close(reader);
    return loaded;
}

bool load_engine_model_geometry(const char* name, IReader* source,
    ModelGeometry& result, std::string& error)
{
    VisualRecord visual;
    if (source)
    {
        if (!load_engine_model_reader(*source, name, visual, error))
            return false;
    }
    else if (!load_engine_model_visual(name, visual, error))
        return false;
    if (decode_engine_model_geometry(visual, result, error))
        return true;
    error = std::string(name && *name ? name : "<reader>.ogf") + " / " + error;
    return false;
}

namespace
{
bool decode_container_geometry(const VisualRecord& visual, pcstr geometry_name, ModelGeometry& result, std::string& error)
{
    CStreamReader* geometry = FS.rs_open("$level$", geometry_name);
    if (!geometry)
    {
        error = std::string("container-backed OGF needs ") + geometry_name;
        return false;
    }
    CStreamReader* vb = geometry->open_chunk(fsL_VB);
    CStreamReader* ib = geometry->open_chunk(fsL_IB);
    CStreamReader* swi = geometry->open_chunk(fsL_SWIS);
    bool loaded = false;
    LevelModel model;
    if (vb && ib)
    {
        xr_vector<uint8_t> vb_bytes(vb->length()), ib_bytes(ib->length());
        vb->r(vb_bytes.data(), vb_bytes.size());
        ib->r(ib_bytes.data(), ib_bytes.size());
        xr_vector<uint8_t> swi_bytes;
        if (swi)
        {
            swi_bytes.resize(swi->length());
            swi->r(swi_bytes.data(), swi_bytes.size());
        }
        loaded = load_container_model({ vb_bytes.data(), vb_bytes.size() }, { ib_bytes.data(), ib_bytes.size() }, visual, model, error,
            {swi_bytes.data(), swi_bytes.size()});
    }
    else
        error = std::string(geometry_name) + " has no vertex or index buffers";
    if (vb)
        vb->close();
    if (ib)
        ib->close();
    if (swi)
        swi->close();
    FS.r_close(geometry);
    if (!loaded)
        return false;
    ModelGeometry converted;
    converted.type = visual.type;
    converted.shader = visual.shader;
    converted.texture = visual.texture;
    converted.mode = classify_surface_material(converted.shader, converted.texture);
    converted.indices = std::move(model.indices);
    converted.windows = std::move(model.windows);
    converted.vertices.reserve(model.vertices.size());
    for (const auto& vertex : model.vertices)
    {
        ModelVertex out;
        std::copy_n(vertex.position, 3, out.position);
        std::copy_n(vertex.normal, 3, out.normal);
        std::copy_n(vertex.uv, 2, out.uv);
        converted.vertices.push_back(out);
    }
    result = std::move(converted);
    error.clear();
    return true;
}

void append_chunk(std::vector<uint8_t>& bytes, uint32_t id, const uint8_t* data, size_t size)
{
    for (uint32_t value : {id, static_cast<uint32_t>(size)})
        for (unsigned i = 0; i < 4; ++i)
            bytes.push_back(static_cast<uint8_t>(value >> (8 * i)));
    if (size)
        bytes.insert(bytes.end(), data, data + size);
}
} // namespace

bool decode_engine_model_geometry(const VisualRecord& visual, ModelGeometry& result, std::string& error)
{
    ModelGeometry decoded;
    if (!decode_model_geometry(visual, decoded, error))
    {
        if ((visual.type != 0 && visual.type != 2 && visual.type != 7 && visual.type != 11) || visual.source.empty())
            return false;
        IReader source(const_cast<uint8_t*>(visual.source.data()), visual.source.size());
        if (!source.find_chunk(OGF_GCONTAINER) || !decode_container_geometry(visual, "level.geom", decoded, error))
            return false;
    }
    if (visual.type == 2)
    {
        IReader source(const_cast<uint8_t*>(visual.source.data()), visual.source.size());
        IReader* fast = source.open_chunk(OGF_FASTPATH);
        if (fast)
        {
            const auto* fast_bytes = static_cast<const uint8_t*>(fast->pointer());
            const size_t fast_size = fast->length();
            IReader* header = source.open_chunk(OGF_HEADER);
            if (!header || !fast->find_chunk(OGF_GCONTAINER) || !fast->find_chunk(OGF_SWIDATA))
            {
                if (header)
                    header->close();
                fast->close();
                error = "progressive OGF fast path is missing its header, geometry or sliding windows";
                return false;
            }
            std::vector<uint8_t> nested;
            append_chunk(nested, OGF_HEADER, static_cast<const uint8_t*>(header->pointer()), header->length());
            nested.insert(nested.end(), fast_bytes, fast_bytes + fast_size);
            header->close();
            fast->close();
            VisualRecord variant;
            if (!parse_ogf_visual({ nested.data(), nested.size() }, variant, error))
                return false;
            variant.texture = visual.texture;
            variant.shader = visual.shader;
            auto fast_geometry = std::make_shared<ModelGeometry>();
            if (!decode_container_geometry(variant, "level.geomX", *fast_geometry, error))
            {
                Msg("! [renderer-vulkan] standalone model fast geometry unavailable: %s; using regular geometry", error.c_str());
                error.clear();
            }
            else
                decoded.fast = std::move(fast_geometry);
        }
    }
    result = std::move(decoded);
    error.clear();
    return true;
}

bool load_engine_motion_files(const std::string& pattern, std::vector<MotionFile>& files, std::string& error)
{
    std::vector<std::string> names;
    if (pattern.find('*') != std::string::npos)
    {
        FS_FileSet found;
        FS.file_list(found, "$game_meshes$", FS_ListFiles, pattern.c_str());
        FS.file_list(found, "$level$", FS_ListFiles, pattern.c_str());
        for (const auto& entry : found)
            names.push_back(entry.name.c_str());
    }
    else
        names.push_back(pattern);

    std::vector<MotionFile> loaded;
    for (const auto& name : names)
    {
        string_path filename;
        if (!FS.exist(filename, "$level$", name.c_str()) && !FS.exist(filename, "$game_meshes$", name.c_str()))
        {
            error = "motion file not found: " + name;
            return false;
        }
        IReader* reader = FS.r_open(filename);
        if (!reader)
        {
            error = "motion file cannot be opened: " + name;
            return false;
        }
        IReader* params = reader->open_chunk(OGF_S_SMPARAMS);
        IReader* motions = reader->open_chunk(OGF_S_MOTIONS);
        std::vector<uint8_t> bytes;
        if (params && motions && params->length() <= UINT32_MAX)
        {
            IReader* count = motions->open_chunk(0);
            std::vector<uint8_t> tracks;
            if (count && count->length() == sizeof(u32))
            {
                append_chunk(tracks, 0, static_cast<const uint8_t*>(count->pointer()), count->length());
                const u32 clip_count = count->r_u32();
                if (clip_count == 0 || clip_count >= 0x3fff)
                    tracks.clear();
                else
                    for (u32 i = 1; i <= clip_count; ++i)
                    {
                        IReader* clip = motions->open_chunk(i);
                        if (!clip || clip->length() > UINT32_MAX)
                            tracks.clear();
                        else if (!tracks.empty())
                            append_chunk(tracks, i, static_cast<const uint8_t*>(clip->pointer()), clip->length());
                        if (clip)
                            clip->close();
                        if (tracks.empty())
                            break;
                    }
            }
            if (count)
                count->close();
            if (!tracks.empty())
            {
                append_chunk(bytes, OGF_S_SMPARAMS, static_cast<const uint8_t*>(params->pointer()), params->length());
                append_chunk(bytes, OGF_S_MOTIONS, tracks.data(), tracks.size());
            }
        }
        if (params)
            params->close();
        if (motions)
            motions->close();
        FS.r_close(reader);
        if (bytes.empty())
        {
            error = "motion file is missing parameters or clips: " + name;
            return false;
        }
        loaded.emplace_back(name, std::move(bytes));
    }
    if (loaded.empty())
    {
        error = "motion file not found: " + pattern;
        return false;
    }
    files = std::move(loaded);
    error.clear();
    return true;
}

bool load_engine_level_models(IReader &level, LevelModelData &result, std::string &error)
{
    error.clear();
    const auto profile = game_profile();
    IReader *header = level.open_chunk(fsL_HEADER);
    const bool valid =
        validate_level_header(profile, header ? LevelBytes{static_cast<const uint8_t *>(header->pointer()), header->length()} : LevelBytes{}, error);
    if (header)
        header->close();
    if (!valid)
        return false;
    IReader *shaders = level.open_chunk(fsL_SHADERS);
    IReader *visuals = level.open_chunk(fsL_VISUALS);
    if (!shaders || !visuals)
    {
        error = std::string(level_profile_name(profile)) + " $level$/level / shader/visual table: missing chunk";
        if (shaders)
            shaders->close();
        if (visuals)
            visuals->close();
        return false;
    }
    CStreamReader* geometry = FS.rs_open("$level$", "level.geom");
    if (!geometry)
    {
        shaders->close();
        visuals->close();
        error = std::string(level_profile_name(profile)) + " $level$/level.geom / VFS open: missing resource";
        return false;
    }
    CStreamReader *vb = geometry->open_chunk(fsL_VB);
    CStreamReader* ib = geometry->open_chunk(fsL_IB);
    CStreamReader* swi = geometry->open_chunk(fsL_SWIS);
    if (!vb || !ib)
        error = "level.geom is missing vertex or index chunks";
    else
    {
        // Stream reader mappings can remap on advance; own the bytes before
        // decoding and let the engine handle archive/decompression semantics.
        xr_vector<uint8_t> vb_bytes(vb->length()), ib_bytes(ib->length());
        vb->r(vb_bytes.data(), vb_bytes.size());
        ib->r(ib_bytes.data(), ib_bytes.size());
        xr_vector<uint8_t> swi_bytes;
        if (swi)
        {
            swi_bytes.resize(swi->length());
            swi->r(swi_bytes.data(), swi_bytes.size());
        }
        CStreamReader* fast = FS.rs_open("$level$", "level.geomX");
        xr_vector<uint8_t> fast_vb_bytes, fast_ib_bytes;
        if (fast)
        {
            CStreamReader* fast_vb = fast->open_chunk(fsL_VB);
            CStreamReader* fast_ib = fast->open_chunk(fsL_IB);
            if (fast_vb && fast_ib)
            {
                fast_vb_bytes.resize(fast_vb->length());
                fast_ib_bytes.resize(fast_ib->length());
                fast_vb->r(fast_vb_bytes.data(), fast_vb_bytes.size());
                fast_ib->r(fast_ib_bytes.data(), fast_ib_bytes.size());
            }
            else
                error = "level.geomX is missing vertex or index chunks";
            if (fast_vb)
                fast_vb->close();
            if (fast_ib)
                fast_ib->close();
            FS.r_close(fast);
        }
        std::vector<uint8_t> visual_bytes;
        if (error.empty() && normalize_engine_visual_chunks(*visuals, true, visual_bytes, error))
        {
            bool skipped_fast_geometry = false;
            load_level_models({static_cast<const uint8_t *>(shaders->pointer()), shaders->length()}, {vb_bytes.data(), vb_bytes.size()},
                              {ib_bytes.data(), ib_bytes.size()}, {visual_bytes.data(), visual_bytes.size()}, result, error,
                              fast_vb_bytes.empty() ? LevelBytes{} : LevelBytes{fast_vb_bytes.data(), fast_vb_bytes.size()},
                              fast_ib_bytes.empty() ? LevelBytes{} : LevelBytes{fast_ib_bytes.data(), fast_ib_bytes.size()},
                              {swi_bytes.data(), swi_bytes.size()}, &skipped_fast_geometry);
            if (skipped_fast_geometry)
                Msg("! [renderer-vulkan] level.geomX buffers are incompatible; using regular level.geom for progressive visuals");
            Msg("[renderer-vulkan] level.decode complete models=%zu visuals=%zu error='%s'",
                result.models.size(), result.visuals.size(), error.c_str());
        }
    }
    Msg("[renderer-vulkan] level.decode closing geometry streams");
    if (vb) vb->close();
    if (ib) ib->close();
    if (swi) swi->close();
    FS.r_close(geometry);
    shaders->close();
    visuals->close();
    Msg("[renderer-vulkan] level.decode streams closed");
    if (!error.empty())
        error = std::string(level_profile_name(profile)) + " $level$/level[.geom/.geomX] / resource decode: " + error;
    return error.empty();
}

bool load_engine_level_visibility(IReader& level, LevelModelData& result, std::string& error)
{
    IReader* portals_chunk = level.open_chunk(fsL_PORTALS);
    IReader* sectors_chunk = level.open_chunk(fsL_SECTORS);
    if (!portals_chunk || !sectors_chunk)
    {
        if (portals_chunk) portals_chunk->close();
        if (sectors_chunk) sectors_chunk->close();
        error = "level has no complete portal and sector tables";
        return false;
    }
    const bool parsed = parse_level_visibility(
        {static_cast<const uint8_t*>(portals_chunk->pointer()), portals_chunk->length()},
        {static_cast<const uint8_t*>(sectors_chunk->pointer()), sectors_chunk->length()},
        result, error);
    portals_chunk->close();
    sectors_chunk->close();
    return parsed;
}
} // namespace xray::render::vulkan
