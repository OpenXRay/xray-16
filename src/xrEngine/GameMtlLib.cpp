#include "stdafx.h"
#include "GameMtlLib.h"
#include "Common/FSMacros.hpp"

CGameMtlLibrary GMLib;
#ifdef _EDITOR
CGameMtlLibrary* PGMLib = nullptr;
#endif

#ifdef DEBUG
const char* SGameMtlPair::dbg_Name()
{
    static string256 nm;
    SGameMtl* M0 = GMLib.GetMaterialByID(GetMtl0());
    SGameMtl* M1 = GMLib.GetMaterialByID(GetMtl1());
    xr_sprintf(nm, sizeof(nm), "Pair: %s - %s", *M0->m_Name, *M1->m_Name);
    return nm;
}
#endif

void SGameMtl::Load(IReader& fs)
{
    R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_MAIN));
    ID = fs.r_u32();
    fs.r_stringZ(m_Name);

    if (fs.find_chunk(GAMEMTL_CHUNK_DESC))
    {
        fs.r_stringZ(m_Desc);
    }

    R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_FLAGS));
    Flags.assign(fs.r_u32());

    R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_PHYSICS));
    fPHFriction = fs.r_float();
    fPHDamping = fs.r_float();
    fPHSpring = fs.r_float();
    fPHBounceStartVelocity = fs.r_float();
    fPHBouncing = fs.r_float();

    R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_FACTORS));
    fShootFactor = fs.r_float();
    fBounceDamageFactor = fs.r_float();
    fVisTransparencyFactor = fs.r_float();
    fSndOcclusionFactor = fs.r_float();

    if (fs.find_chunk(GAMEMTL_CHUNK_FACTORS_MP))
        fShootFactorMP = fs.r_float();
    else
        fShootFactorMP = fShootFactor;

    if (fs.find_chunk(GAMEMTL_CHUNK_FLOTATION))
        fFlotationFactor = fs.r_float();

    if (fs.find_chunk(GAMEMTL_CHUNK_INJURIOUS))
        fInjuriousSpeed = fs.r_float();

    if (fs.find_chunk(GAMEMTL_CHUNK_DENSITY))
        fDensityFactor = fs.r_float();
}

CGameMtlLibrary::CGameMtlLibrary()
{
    material_index = 0;
    material_pair_index = 0;
#if !defined(_EDITOR) && !defined(GM_NON_GAME)
    GEnv.PGMLib = &GMLib;
#endif
}

void CGameMtlLibrary::Load()
{
    string_path name;
    if (!FS.exist(name, _game_data_, GAMEMTL_FILENAME))
    {
        Log("! Can't find game material file: ", name);
        return;
    }

    R_ASSERT(material_pairs.empty());
    R_ASSERT(materials.empty());

    IReader* F = FS.r_open(name);
    IReader& fs = *F;

    R_ASSERT(fs.find_chunk(GAMEMTLS_CHUNK_VERSION));
    u16 version = fs.r_u16();
    if (GAMEMTL_CURRENT_VERSION != version)
    {
        Log("CGameMtlLibrary: invalid version. Library can't load.");
        FS.r_close(F);
        return;
    }

    R_ASSERT(fs.find_chunk(GAMEMTLS_CHUNK_AUTOINC));
    material_index = fs.r_u32();
    material_pair_index = fs.r_u32();

    materials.reserve(material_index);
    material_pairs.reserve(material_pair_index);

    IReader* OBJ = fs.open_chunk(GAMEMTLS_CHUNK_MTLS);
    if (OBJ)
    {
        u32 count;
        for (IReader* O = OBJ->open_chunk_iterator(count); O; O = OBJ->open_chunk_iterator(count, O))
        {
            SGameMtl* M = xr_new<SGameMtl>();
            M->Load(*O);
            materials.push_back(M);
        }
        OBJ->close();
    }

    OBJ = fs.open_chunk(GAMEMTLS_CHUNK_MTLS_PAIR);
    if (OBJ)
    {
        u32 count;
        for (IReader* O = OBJ->open_chunk_iterator(count); O; O = OBJ->open_chunk_iterator(count, O))
        {
            SGameMtlPair* M = xr_new<SGameMtlPair>(this);
            M->Load(*O);
            material_pairs.push_back(M);
        }
        OBJ->close();
    }
    u32 mtlCount = materials.size();
    material_pairs_rt.resize(mtlCount * mtlCount, 0);
    for (auto& mtlPair : material_pairs)
    {
        int idx0 = GetMaterialIdx(mtlPair->mtl0) * mtlCount + GetMaterialIdx(mtlPair->mtl1);
        int idx1 = GetMaterialIdx(mtlPair->mtl1) * mtlCount + GetMaterialIdx(mtlPair->mtl0);
        material_pairs_rt[idx0] = mtlPair;
        material_pairs_rt[idx1] = mtlPair;
    }
    FS.r_close(F);
}
