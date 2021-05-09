// xrCDB.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "xrCDB.h"
#include "xrCore/Threading/Lock.hpp"

namespace Opcode
{
#include "OPCODE/OPC_TreeBuilders.h"
} // namespace Opcode

using namespace CDB;
using namespace Opcode;

// Model building
MODEL::MODEL() :
#ifdef CONFIG_PROFILE_LOCKS
    pcs(xr_new<Lock>(MUTEX_PROFILE_ID(MODEL)))
#else
    pcs(xr_new<Lock>())
#endif // CONFIG_PROFILE_LOCKS
{
    tree = 0;
    tris = 0;
    tris_count = 0;
    verts = 0;
    verts_count = 0;
    status = S_INIT;
}
MODEL::~MODEL()
{
    syncronize(); // maybe model still in building
    status = S_INIT;
    xr_delete(tree);
    xr_free(tris);
    tris_count = 0;
    xr_free(verts);
    verts_count = 0;
    delete pcs;
}

void MODEL::syncronize_impl() const
{
    Log("! WARNING: syncronized CDB::query");
    Lock* C = pcs;
	C->Enter();
	C->Leave();
}

struct BTHREAD_params
{
    MODEL* M;
    Fvector* V;
    int Vcnt;
    TRI* T;
    int Tcnt;
    build_callback* BC;
    void* BCP;
};

void MODEL::build_thread(void* params)
{
    _initialize_cpu_thread();
    FPU::m64r();
    BTHREAD_params P = *((BTHREAD_params*)params);
    P.M->pcs->Enter();
    P.M->build_internal(P.V, P.Vcnt, P.T, P.Tcnt, P.BC, P.BCP);
    P.M->status = S_READY;
    P.M->pcs->Leave();
    // Msg						("* xrCDB: cform build completed, memory usage: %d K",P.M->memory()/1024);
}

void MODEL::build(Fvector* V, int Vcnt, TRI* T, int Tcnt, build_callback* bc, void* bcp)
{
    R_ASSERT(S_INIT == status);
    R_ASSERT((Vcnt >= 4) && (Tcnt >= 2));

    _initialize_cpu_thread();
#ifdef _EDITOR
    build_internal(V, Vcnt, T, Tcnt, bc, bcp);
#else
    if (!strstr(Core.Params, "-mt_cdb"))
    {
        build_internal(V, Vcnt, T, Tcnt, bc, bcp);
        status = S_READY;
    }
    else
    {
        BTHREAD_params P = {this, V, Vcnt, T, Tcnt, bc, bcp};
        Threading::SpawnThread(build_thread, "CDB-construction", 0, &P);
        while (S_INIT == status)
        {
            if (status != S_INIT)
                break;
            Sleep(5);
        }
    }
#endif
}

void MODEL::build_internal(Fvector* V, int Vcnt, TRI* T, int Tcnt, build_callback* bc, void* bcp)
{
    // verts
    verts_count = Vcnt;
    verts = xr_alloc<Fvector>(verts_count);
    CopyMemory(verts, V, verts_count * sizeof(Fvector));

    // tris
    tris_count = Tcnt;
    tris = xr_alloc<TRI>(tris_count);
    CopyMemory(tris, T, tris_count * sizeof(TRI));

    // callback
    if (bc)
        bc(verts, Vcnt, tris, Tcnt, bcp);

    // Release data pointers
    status = S_BUILD;

    // Allocate temporary "OPCODE" tris + convert tris to 'pointer' form
    u32* temp_tris = xr_alloc<u32>(tris_count * 3);
    if (0 == temp_tris)
    {
        xr_free(verts);
        xr_free(tris);
        return;
    }
    u32* temp_ptr = temp_tris;
    for (int i = 0; i < tris_count; i++)
    {
        *temp_ptr++ = tris[i].verts[0];
        *temp_ptr++ = tris[i].verts[1];
        *temp_ptr++ = tris[i].verts[2];
    }

    // Build a non quantized no-leaf tree
    OPCODECREATE OPCC;
    OPCC.NbTris = tris_count;
    OPCC.NbVerts = verts_count;
    OPCC.Tris = (unsigned*)temp_tris;
    OPCC.Verts = (Point*)verts;
    OPCC.Rules = SPLIT_COMPLETE | SPLIT_SPLATTERPOINTS | SPLIT_GEOMCENTER;
    OPCC.NoLeaf = true;
    OPCC.Quantized = false;

    tree = xr_new<OPCODE_Model>();
    if (!tree->Build(OPCC))
    {
        xr_free(verts);
        xr_free(tris);
        xr_free(temp_tris);
        return;
    };

    // Free temporary tris
    xr_free(temp_tris);
    return;
}

u32 MODEL::memory()
{
    if (S_BUILD == status)
    {
        Msg("! xrCDB: model still isn't ready");
        return 0;
    }
    u32 V = verts_count * sizeof(Fvector);
    u32 T = tris_count * sizeof(TRI);
    return tree->GetUsedBytes() + V + T + sizeof(*this) + sizeof(*tree);
}

bool MODEL::serialize(pcstr fileName) const
{
    IWriter* wstream = FS.w_open(fileName);
    if (!wstream)
        return false;

    wstream->w_u32(version);
    wstream->w_u32(verts_count);
    wstream->w(verts, sizeof(Fvector) * verts_count);
    wstream->w_u32(tris_count);
    wstream->w(tris, sizeof(TRI) * tris_count);

    if (tree)
        tree->Save(wstream);
    FS.w_close(wstream);
    return true;
}

bool MODEL::deserialize(pcstr fileName)
{
    IReader* rstream = FS.r_open(fileName);
    if (!rstream)
        return false;

    if (version != rstream->r_u32())
    {
        FS.r_close(rstream);
        return false;
    }

    xr_free(verts);
    xr_free(tris);
    xr_free(tree);

    verts_count = rstream->r_u32();
    verts = xr_alloc<Fvector>(verts_count);
    const u32 vertsSize = verts_count * sizeof(Fvector);
    CopyMemory(verts, rstream->pointer(), vertsSize);
    rstream->advance(vertsSize);

    tris_count = rstream->r_u32();
    tris = xr_alloc<TRI>(tris_count);
    const u32 trisSize = tris_count * sizeof(TRI);
    CopyMemory(tris, rstream->pointer(), trisSize);
    rstream->advance(trisSize);

    tree = xr_new<OPCODE_Model>();
    tree->Load(rstream);
    FS.r_close(rstream);
    status = S_READY;
    return true;
}

// This is the constructor of a class that has been exported.
// see xrCDB.h for the class definition
COLLIDER::COLLIDER()
{
    ray_mode = 0;
    box_mode = 0;
    frustum_mode = 0;
}

COLLIDER::~COLLIDER() { r_free(); }
RESULT& COLLIDER::r_add()
{
    return rd.emplace_back(RESULT());
}

void COLLIDER::r_free() { rd.clear(); }
