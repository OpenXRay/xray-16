// dxRender_Visual.cpp: implementation of the dxRender_Visual class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "FBasicVisual.h"
#include "xrCore/FMesh.hpp"
#include "xrEngine/xr_object.h"
#include "Layers/xrRender/Materials/ShaderInfo.h"

namespace xray::render::fg
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
        vis.box.set(hdr.bb.min, hdr.bb.max);
        vis.sphere.set(hdr.bs.c, hdr.bs.r);
    }
    else
    {
        FATAL("Invalid visual");
    }

    if (data->find_chunk(OGF_TEXTURE))
    {
        string256 fnT, fnS;
        data->r_stringZ(fnT, sizeof(fnT));
        data->r_stringZ(fnS, sizeof(fnS));

        shaderName = fnS;
        textureName = fnT;
    }
    else if (hdr.shader_id)
    {
        xray::render::shader_info::GetCompiledShaderNames(hdr.shader_id, shaderName, textureName);
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
    PCOPY(shaderName);
    PCOPY(textureName);
    PCOPY(vis);

    static u32 copyCount = 0;
    if (++copyCount <= 5) {
        Msg("* [Visual::Copy] from=%p to=%p tex='%s' shader='%s'",
            pFrom, this, pFrom->textureName.c_str(), pFrom->shaderName.c_str());
    }
#ifdef _EDITOR
    PCOPY(desc);
#endif
#ifdef DEBUG
    PCOPY(dbg_name);
#endif
}
}
