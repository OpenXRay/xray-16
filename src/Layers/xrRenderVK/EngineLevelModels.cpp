#include "xrEngine/stdafx.h"
#include "EngineLevelModels.h"

#include "Common/LevelStructure.hpp"
#include "xrCore/stream_reader.h"

namespace xray::render::vulkan
{
bool load_engine_level_models(IReader& level, LevelModelData& result, std::string& error)
{
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
        load_level_models({static_cast<const uint8_t*>(shaders->pointer()), shaders->length()},
            {vb_bytes.data(), vb_bytes.size()}, {ib_bytes.data(), ib_bytes.size()},
            {static_cast<const uint8_t*>(visuals->pointer()), visuals->length()}, result, error);
    }
    if (vb) vb->close();
    if (ib) ib->close();
    FS.r_close(geometry);
    shaders->close();
    visuals->close();
    return error.empty();
}
}
