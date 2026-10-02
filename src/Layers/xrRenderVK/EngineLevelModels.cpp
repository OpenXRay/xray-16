#include "xrEngine/stdafx.h"
#include "EngineLevelModels.h"

#include "Common/LevelStructure.hpp"
#include "xrCore/FMesh.hpp"
#include "xrCore/stream_reader.h"

#include <algorithm>
#include <memory>

namespace xray::render::vulkan
{
bool load_engine_visual_catalog(IReader& level, std::vector<VisualRecord>& result, std::string& error)
{
    IReader* visuals = level.open_chunk(fsL_VISUALS);
    if (!visuals)
    {
        error = "level has no OGF visual table";
        return false;
    }
    const bool loaded = parse_level_visuals(
        {static_cast<const uint8_t*>(visuals->pointer()), visuals->length()}, result, error);
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
    const bool loaded = parse_ogf_visual(
        {static_cast<const uint8_t*>(reader->pointer()), reader->length()}, result, error);
    FS.r_close(reader);
    return loaded;
}

bool load_engine_model_geometry(const char* name, IReader* source,
    ModelGeometry& result, std::string& error)
{
    VisualRecord visual;
    if (source)
    {
        if (!parse_ogf_visual({static_cast<const uint8_t*>(source->pointer()),
                source->length()}, visual, error)) return false;
    }
    else if (!load_engine_model_visual(name, visual, error)) return false;
    return decode_engine_model_geometry(visual, result, error);
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
    bool loaded = false;
    LevelModel model;
    if (vb && ib)
    {
        xr_vector<uint8_t> vb_bytes(vb->length()), ib_bytes(ib->length());
        vb->r(vb_bytes.data(), vb_bytes.size());
        ib->r(ib_bytes.data(), ib_bytes.size());
        loaded = load_container_model({ vb_bytes.data(), vb_bytes.size() }, { ib_bytes.data(), ib_bytes.size() }, visual, model, error);
    }
    else
        error = std::string(geometry_name) + " has no vertex or index buffers";
    if (vb)
        vb->close();
    if (ib)
        ib->close();
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
    for (uint32_t value : { id, static_cast<uint32_t>(size) })
        for (unsigned i = 0; i < 4; ++i)
            bytes.push_back(static_cast<uint8_t>(value >> (8 * i)));
    bytes.insert(bytes.end(), data, data + size);
}
} // namespace

bool decode_engine_model_geometry(const VisualRecord& visual, ModelGeometry& result, std::string& error)
{
    ModelGeometry decoded;
    if (!decode_model_geometry(visual, decoded, error))
    {
        if ((visual.type != 0 && visual.type != 2) || visual.source.empty())
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
                return false;
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

bool load_engine_level_models(IReader& level, LevelModelData& result, std::string& error)
{
    error.clear();
    IReader* shaders = level.open_chunk(fsL_SHADERS);
    IReader* visuals = level.open_chunk(fsL_VISUALS);
    if (!shaders || !visuals)
    {
        error = "level is missing shader or visual chunks";
        if (shaders) shaders->close();
        if (visuals) visuals->close();
        return false;
    }
    CStreamReader* geometry = FS.rs_open("$level$", "level.geom");
    if (!geometry)
    {
        shaders->close();
        visuals->close();
        error = "level.geom is missing";
        return false;
    }
    CStreamReader* vb = geometry->open_chunk(fsL_VB);
    CStreamReader* ib = geometry->open_chunk(fsL_IB);
    if (!vb || !ib)
        error = "level.geom is missing vertex or index chunks";
    else
    {
        // Stream reader mappings can remap on advance; own the bytes before
        // decoding and let the engine handle archive/decompression semantics.
        xr_vector<uint8_t> vb_bytes(vb->length()), ib_bytes(ib->length());
        vb->r(vb_bytes.data(), vb_bytes.size());
        ib->r(ib_bytes.data(), ib_bytes.size());
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
        if (error.empty())
            load_level_models({ static_cast<const uint8_t*>(shaders->pointer()), shaders->length() }, { vb_bytes.data(), vb_bytes.size() },
                { ib_bytes.data(), ib_bytes.size() }, { static_cast<const uint8_t*>(visuals->pointer()), visuals->length() }, result, error,
                fast_vb_bytes.empty() ? LevelBytes{} : LevelBytes{ fast_vb_bytes.data(), fast_vb_bytes.size() },
                fast_ib_bytes.empty() ? LevelBytes{} : LevelBytes{ fast_ib_bytes.data(), fast_ib_bytes.size() });
    }
    if (vb) vb->close();
    if (ib) ib->close();
    FS.r_close(geometry);
    shaders->close();
    visuals->close();
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
}
