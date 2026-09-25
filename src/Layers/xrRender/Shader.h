// Shader.h: interface for the CShader class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCore/xr_resource.h"

#include "SH_Atomic.h"
#include "SH_Texture.h"

namespace xray::render::fg
{
using sh_list = xr_vector<shared_str>;

#pragma pack(push, 4)

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SGeometry : public xr_resource_flagged
{
    ref_declaration dcl;
    VertexBufferHandle vb;
    IndexBufferHandle ib;
    u32 vb_stride;
    SGeometry() = default;
    ~SGeometry();
};

struct ECORE_API resptrcode_geom : public resptr_base<SGeometry>
{
    void create(const VertexElement* decl, VertexBufferHandle vb, IndexBufferHandle ib);
    void create(u32 FVF, VertexBufferHandle vb, IndexBufferHandle ib);
    void create_logical(const VertexElement* decl);
    void destroy() { _set(nullptr); }
    u32 stride() const { return _get()->vb_stride; }
};

typedef resptr_core<SGeometry, resptrcode_geom> ref_geom;

#pragma pack(pop)
} // namespace xray::render::fg
