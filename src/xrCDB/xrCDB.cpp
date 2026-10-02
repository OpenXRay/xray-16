// xrCDB.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "xrCDB.h"
#include "xrCore/Threading/Lock.hpp"



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
        Threading::SpawnThread("CDB-construction", [this, V, Vcnt, T, Tcnt, bc, bcp]
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

    tree = xr_new<ModelTree>();
    R_ASSERT2(tree->Build(verts, verts_count, tris, tris_count), "Invalid collision geometry");
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

    syncronize();

    const xr_string cacheName = xr_string(fileName) + ".opcode13";
    IWriter* wstream = FS.w_open(cacheName.c_str());
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

    const xr_string cacheName = xr_string(fileName) + ".opcode13";
    fileName = cacheName.c_str();
    IReader* rstream = FS.r_open(fileName);
    if (!rstream)
        return false;


    if (rstream->elapsed() < static_cast<intptr_t>(sizeof(u32)))
    {
        FS.r_close(rstream);
        return false;
    }

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
    if (rstream->elapsed() < static_cast<intptr_t>(3 * sizeof(u32)))
    {
        FS.r_close(rstream);
        return false;
    }
    const u32 modelCrc = rstream->r_u32();

    const auto integrityPointer = rstream->pointer();
    const u32 vertexCount = rstream->r_u32();
    const u32 triangleCount = rstream->r_u32();
    const size_t remaining = rstream->elapsed();
    if (vertexCount < 4 || triangleCount < 2 || vertexCount > remaining / sizeof(Fvector))
    {
        FS.r_close(rstream);
        return false;
    }
    const size_t vertsSize = static_cast<size_t>(vertexCount) * sizeof(Fvector);
    if (triangleCount > (remaining - vertsSize) / sizeof(TRI))
    {
        FS.r_close(rstream);
        return false;
    }
    const size_t trisSize = static_cast<size_t>(triangleCount) * sizeof(TRI);
    const size_t treeSize = sizeof(vertexCount) + sizeof(triangleCount) + vertsSize + trisSize;

    const u32 actualModelCrc = skipCrc32Check ? modelCrc : crc32(integrityPointer, treeSize);
    if (modelCrc != actualModelCrc)
    {
        FS.r_close(rstream);
        return false;
    }

    xr_free(verts);
    xr_free(tris);
    xr_delete(tree);

    verts_count = vertexCount;
    tris_count = triangleCount;
    verts = xr_alloc<Fvector>(verts_count);
    tris = xr_alloc<TRI>(tris_count);
    tree = xr_new<ModelTree>();

    CopyMemory(verts, rstream->pointer(), vertsSize);
    rstream->advance(vertsSize);

    CopyMemory(tris, rstream->pointer(), trisSize);
    rstream->advance(trisSize);

    const bool success = tree->Load(rstream, tris_count, true, skipCrc32Check);
    if (success)
        status = S_READY;

    FS.r_close(rstream);
    return success;
}

void MODEL::deserialize_tree(IReader* rstream)
{
    R_ASSERT(rstream);

    xr_delete(tree);

    tree = xr_new<ModelTree>();

    // Load the OPCODE tree
    const bool restored = tree->Load(rstream, tris_count, false, true);
    const bool success = restored || tree->Build(verts, verts_count, tris, tris_count);
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
    return V + T + sizeof(*this) + (tree ? tree->GetUsedBytes() + sizeof(*tree) : 0);
}

COLLIDER::~COLLIDER() { r_free(); }
RESULT& COLLIDER::r_add()
{
    return rd.emplace_back(RESULT());
}

void COLLIDER::r_free() { rd.clear(); }
