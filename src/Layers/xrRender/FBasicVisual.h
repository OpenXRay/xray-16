#pragma once

#include "xrEngine/vis_common.h"

#include "Include/xrRender/RenderVisual.h"

#define VLOAD_NOVERTICES (1 << 0)

class IParticleCustom;

namespace xray::render::fg
{
// The class itself
class CKinematicsAnimated;
class CKinematics;

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

    // GPU-driven rendering: VB/IB pool IDs for mega-buffer lookup
    // Set during level load, used by ProcessVisualGeometry to compute mega-buffer offsets
    u32 vbPoolID{UINT32_MAX};   // Index into GPUCullingManager::m_vbPools
    u32 ibPoolID{UINT32_MAX};   // Index into GPUCullingManager::m_ibPools
    bool useAlternativeGeom{false};  // True if using fast/alternative geometry

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
        vbPoolID        = UINT32_MAX;
        ibPoolID        = UINT32_MAX;
        useAlternativeGeom = false;
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

    // FrameGraph: deferred shader compilation (store names, compile on-demand)
    shared_str shaderName;   // e.g., "models\\model"
    shared_str textureName;  // e.g., "wood\\wood1"

    // DX12: Index into CRender::CompiledLevelShaders for precompiled PSO lookup
    u32 shader_id{UINT32_MAX};

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
} // namespace xray::render::fg
