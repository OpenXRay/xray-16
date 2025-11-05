// dxRender_Visual.cpp: implementation of the dxRender_Visual class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "FBasicVisual.h"
#include "xrCore/FMesh.hpp"

#include <algorithm>

namespace xray::render::RENDER_NAMESPACE
{
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IRender_Mesh::~IRender_Mesh()
{
    rm_geom.destroy();
    _RELEASE(p_rm_Vertices);
    _RELEASE(p_rm_Indices);
}

dxRender_Visual::dxRender_Visual()
{
    Type = 0;
    shader = nullptr;
    vis.clear();
}

dxRender_Visual::~dxRender_Visual() {}
void dxRender_Visual::Release() {}
// CStatTimer						tscreate;

void dxRender_Visual::Load(const char* N, IReader* data, u32)
{
#ifdef DEBUG
    dbg_name = N;
#endif

    // header
    VERIFY(data);
    ogf_header hdr;
    if (data->r_chunk_safe(OGF_HEADER, &hdr, sizeof(hdr)))
    {
        R_ASSERT2(hdr.format_version == xrOGF_FormatVersion, "Invalid visual version");
        Type = hdr.type;
        if (hdr.shader_id)
            shader = RImplementation.getShader(hdr.shader_id);
        vis.box.set(hdr.bb.min, hdr.bb.max);
        vis.sphere.set(hdr.bs.c, hdr.bs.r);
    }
    else
    {
        FATAL("Invalid visual");
    }

    // Shader
    if (data->find_chunk(OGF_TEXTURE))
    {
        string256 fnT, fnS;
        data->r_stringZ(fnT, sizeof(fnT));
        data->r_stringZ(fnS, sizeof(fnS));
        shader.create(fnS, fnT);
    }

// desc
#ifdef _EDITOR
    if (data->find_chunk(OGF_S_DESC))
        desc.Load(*data);
#endif
}

#define PCOPY(a) a = pFrom->a
void dxRender_Visual::Copy(dxRender_Visual* pFrom)
{
    PCOPY(Type);
    PCOPY(shader);
    PCOPY(vis);
#ifdef _EDITOR
    PCOPY(desc);
#endif
#ifdef DEBUG
    PCOPY(dbg_name);
#endif
}

bool dxRender_Visual::AcquirePaletteDumpTicket()
{
    if (!GEnv.Render)
        return false;

    if (GEnv.Render->IsOzzPaletteDebugDumpEnabled())
        return true;

    return GEnv.Render->ConsumeOzzPaletteDebugDumpRequest();
}

void dxRender_Visual::DumpPaletteLog(const char* tag, const char* label, const xr_vector<Fmatrix>& palette) const
{
    if (!tag)
        tag = "visual";

    const u32 bone_count = static_cast<u32>(palette.size());

    const char* visual_label = label;
#ifdef DEBUG
    if ((!visual_label || !visual_label[0]) && dbg_name.size())
        visual_label = dbg_name.c_str();
#endif
    if (!visual_label || !visual_label[0])
        visual_label = "<visual>";

    Msg("[%s][palette] visual=%s bones=%u", tag, visual_label, bone_count);

    const u32 print_count = std::min(bone_count, kPaletteDumpMaxBones);
    for (u32 idx = 0; idx < print_count; ++idx)
    {
        const Fmatrix& bone = palette[idx];
        Msg("[%s][palette] bone[%u] i(%.3f %.3f %.3f) j(%.3f %.3f %.3f) k(%.3f %.3f %.3f) c(%.3f %.3f %.3f)", tag, idx,
            bone.i.x, bone.i.y, bone.i.z,
            bone.j.x, bone.j.y, bone.j.z,
            bone.k.x, bone.k.y, bone.k.z,
            bone.c.x, bone.c.y, bone.c.z);
    }

    if (bone_count > print_count)
        Msg("[%s][palette] ... (omitted %u bones)", tag, bone_count - print_count);
}

void dxRender_Visual::DebugDumpPalette(const xr_vector<Fmatrix>& palette) const
{
    if (!AcquirePaletteDumpTicket())
        return;

    DumpPaletteLog("visual", nullptr, palette);
}
} // namespace xray::render::RENDER_NAMESPACE
