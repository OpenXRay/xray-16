#ifndef FBasicVisualH
#define FBasicVisualH
#pragma once

#include "xrEngine/vis_common.h"

#include "Include/xrRender/RenderVisual.h"

#define VLOAD_NOVERTICES (1 << 0)

// The class itself
class CKinematicsAnimated;
class CKinematics;
class IParticleCustom;

struct IRender_Mesh
{
    // format
    ref_geom rm_geom;

    // verts
    VertexStagingBuffer* p_rm_Vertices;
    u32 vBase;
    u32 vCount;
    u32 vStride{};

    // indices
    IndexStagingBuffer* p_rm_Indices;
    u32 iBase;
    u32 iCount;
    u32 dwPrimitives;

    IRender_Mesh()
    {
        p_rm_Vertices   = nullptr;
        vBase           = 0;
        vCount          = 0;
        p_rm_Indices    = nullptr;
        iBase           = 0;
        iCount          = 0;
        dwPrimitives    = 0;
        rm_geom         = nullptr;
    }
    virtual ~IRender_Mesh();

private:
    IRender_Mesh(const IRender_Mesh& other);
    void operator=(const IRender_Mesh& other);
};

// The class itself
class ECORE_API dxRender_Visual : public IRenderVisual
{
public:
#ifdef _EDITOR
    ogf_desc desc;
#endif
#ifdef DEBUG
    shared_str dbg_name;
    virtual shared_str getDebugName() { return dbg_name; }
#endif
public:
    // Common data for rendering
    u32 Type; // visual's type
    vis_data vis; // visibility-data
    ref_shader shader; // pipe state, shared

    virtual void Render(CBackend& cmd_list, float /*LOD*/, bool use_fast_geo) {} // LOD - Level Of Detail  [0..1], Ignored
    virtual void Load(const char* N, IReader* data, u32 dwFlags);
    virtual void Release(); // Shared memory release
    virtual void Copy(dxRender_Visual* from);
    virtual void Spawn(){};
    virtual void Depart(){};

    //	virtual	CKinematics*		dcast_PKinematics			()				{ return 0;	}
    //	virtual	CKinematicsAnimated*dcast_PKinematicsAnimated	()				{ return 0;	}
    //	virtual IParticleCustom*	dcast_ParticleCustom		()				{ return 0;	}

    virtual vis_data& getVisData() { return vis; }
    u32 getType() const override { return Type; }
    dxRender_Visual();
    virtual ~dxRender_Visual();
};

#endif // !FBasicVisualH
