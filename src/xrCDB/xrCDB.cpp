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
    xr_delete(pcs);
}

void MODEL::syncronize_impl() const
{
    Log("! WARNING: syncronized CDB::query");
    Lock* C = pcs;
    C->Enter();
    C->Leave();
}

void MODEL::build(Fvector* V, u32 Vcnt, TRI* T, u32 Tcnt, build_callback* bc, void* bcp)
{
    ZoneScoped;

    R_ASSERT(S_INIT == status);
    R_ASSERT((Vcnt >= 4) && (Tcnt >= 2));

    _initialize_cpu_thread();

    if (!strstr(Core.Params, "-mt_cdb"))
    {
        build_internal(V, Vcnt, T, Tcnt, bc, bcp);
        status = S_READY;
    }
    else
    {
        Threading::SpawnThread("CDB-construction", [&, this]
        {
            ScopeLock lock{ pcs };
            build_internal(V, Vcnt, T, Tcnt, bc, bcp);
            status = S_READY;
            // Msg("* xrCDB: cform build completed, memory usage: %d K", memory() / 1024);
        });

        while (S_INIT == status)
        {
            if (status != S_INIT)
                break;
            Sleep(5);
        }
    }
}

void MODEL::build_internal(Fvector* V, u32 Vcnt, TRI* T, u32 Tcnt, build_callback* bc, void* bcp)
{
    ZoneScoped;

    xr_free(verts);
    xr_free(tris);
    xr_delete(tree);

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
    for (u32 i = 0; i < tris_count; i++)
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
}

void MODEL::load_geom(Fvector* V, u32 Vcnt, TRI* T, u32 Tcnt)
{
    xr_free(verts);
    xr_free(tris);

    // verts
    verts_count = Vcnt;
    verts = xr_alloc<Fvector>(verts_count);
    CopyMemory(verts, V, static_cast<size_t>(verts_count) * sizeof(Fvector));

    // tris
    tris_count = Tcnt;
    tris = xr_alloc<TRI>(tris_count);
    CopyMemory(tris, T, static_cast<size_t>(tris_count) * sizeof(TRI));
}

/*
    Serialization/Deserialization

    Data layout of the cache file:
    [u32] crc32 of the model file (e.g. level.cform)
    [...] user-specific data (e.g. CLevel::LoadGameSpecificCFORMSerialize writes crc32 of gamemtl.xr)
    [u32] crc32 of the MODEL (4 records below)
    [u32] vertex count
    [u32] index count
    [...] vertices themselves
    [...] indices themselves
    [...] OPCODE tree
*/
bool MODEL::serialize(pcstr fileName, serialize_callback callback /*= nullptr*/) const
{
    ZoneScoped;

    IWriter* wstream = FS.w_open(fileName);
    if (!wstream)
        return false;

    // 1. Source file checksum
    wstream->w_u32(model_crc32);

    // 2. User-specific data (e.g. GameSpecificCFORM)
    if (callback)
        callback(*wstream);

    // 3. MODEL checksum and contents
    auto crc = crc32(&verts_count, sizeof(verts_count));
    crc      = crc32(&tris_count, sizeof(tris_count), crc);
    crc      = crc32(verts, sizeof(Fvector) * verts_count, crc);
    crc      = crc32(tris, sizeof(TRI) * tris_count, crc);

    wstream->w_u32(crc);
    wstream->w_u32(verts_count);
    wstream->w_u32(tris_count);
    wstream->w(verts, sizeof(Fvector) * verts_count);
    wstream->w(tris, sizeof(TRI) * tris_count);

    // 4. OPCODE tree
    if (tree)
        tree->Save(wstream);

    FS.w_close(wstream);
    return true;
}

bool MODEL::deserialize(pcstr fileName, bool skipCrc32Check /*= false*/, deserialize_callback callback /*= nullptr*/)
{
    ZoneScoped;

    IReader* rstream = FS.r_open(fileName);
    if (!rstream)
        return false;

    // 1. Check that model's source file didn't changed
    if (model_crc32 != rstream->r_u32())
    {
        FS.r_close(rstream);
        return false;
    }

    // 2. User-specific data check (e.g. GameSpecificCFORM)
    if (callback && !callback(*rstream))
    {
        FS.r_close(rstream);
        return false;
    }

    // 3. Check MODEL's integrity and load it
    const u32 modelCrc = rstream->r_u32();

    const auto integrityPointer = rstream->pointer();
    verts_count = rstream->r_u32();
    tris_count = rstream->r_u32();

    const size_t vertsSize = static_cast<size_t>(verts_count) * sizeof(Fvector);
    const size_t trisSize = static_cast<size_t>(tris_count) * sizeof(TRI);
    const size_t treeSize = sizeof(verts_count) + sizeof(tris_count) + vertsSize + trisSize;
    if (treeSize > rstream->elapsed())
    {
        FS.r_close(rstream);
        return false;
    }

    const u32 actualModelCrc = skipCrc32Check ? modelCrc : crc32(integrityPointer, treeSize);
    if (modelCrc != actualModelCrc)
    {
        FS.r_close(rstream);
        return false;
    }

    xr_free(verts);
    xr_free(tris);
    xr_delete(tree);

    verts = xr_alloc<Fvector>(verts_count);
    tris = xr_alloc<TRI>(tris_count);
    tree = xr_new<OPCODE_Model>();

    CopyMemory(verts, rstream->pointer(), vertsSize);
    rstream->advance(vertsSize);

    CopyMemory(tris, rstream->pointer(), trisSize);
    rstream->advance(trisSize);

    // 4. Load the OPCODE tree
    const bool success = tree->Load(rstream);
    if (success)
        status = S_READY;

    FS.r_close(rstream);
    return success;
}

void MODEL::deserialize_tree(IReader* rstream)
{
    R_ASSERT(rstream);

    xr_delete(tree);

    tree = xr_new<OPCODE_Model>();

    // Load the OPCODE tree
    const bool success = tree->Load(rstream, true, false);
    if (success)
        status = S_READY;
}

size_t MODEL::memory()
{
    if (S_BUILD == status)
    {
        Msg("! xrCDB: model still isn't ready");
        return 0;
    }
    size_t V = static_cast<size_t>(verts_count) * sizeof(Fvector);
    size_t T = static_cast<size_t>(tris_count) * sizeof(TRI);
    return tree->GetUsedBytes() + V + T + sizeof(*this) + sizeof(*tree);
}

COLLIDER::~COLLIDER() { r_free(); }
RESULT& COLLIDER::r_add()
{
    return rd.emplace_back(RESULT());
}

void COLLIDER::r_free() { rd.clear(); }
