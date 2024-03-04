#include "stdafx.h"
#include "GameMtlLib.h"
#include "Common/FSMacros.hpp"

CGameMtlLibrary GMLib;

#ifdef DEBUG
const char* SGameMtlPair::dbg_Name() const
{
    static string256 nm;
    const SGameMtl* M0 = GMLib.GetMaterialByID(GetMtl0());
    const SGameMtl* M1 = GMLib.GetMaterialByID(GetMtl1());
    xr_sprintf(nm, sizeof(nm), "Pair: %s - %s", *M0->m_Name, *M1->m_Name);
    return nm;
}
#endif

/*
materials\asphalt
materials\bricks
materials\bush
materials\bush_sux
materials\cloth
materials\concrete
materials\death
materials\dirt
materials\earth
materials\earth_death
materials\earth_slide
materials\flooring_tile
materials\glass
materials\grass
materials\gravel
materials\metal
materials\metal_pipe
materials\metal_plate
materials\sand
materials\setka_rabica
materials\shifer
materials\stucco
materials\tin
materials\tree_trunk
materials\water
materials\water_radiation
materials\wood
materials\wooden_board

objects\barrel
objects\bottle
objects\bullet
objects\car_cabine
objects\car_wheel
objects\clothes
objects\concrete_box
objects\dead_body
objects\fuel_can
objects\glass
objects\knife
objects\large_furniture
objects\large_metal_trash
objects\large_weapon
objects\metal_box
objects\monster_body
objects\small_box
objects\small_metal_trash
objects\small_weapon
objects\tin_can
*/

bool assign_default_acoustics(pcstr material, SGameMtl::MtlAcoustics& acoustics)
{
    constexpr std::tuple<u32, SGameMtl::MtlAcoustics> defaults[] =
    {
        { "default"_hash,                   { { 0.10f, 0.20f, 0.30f }, 0.05f, { 0.100f, 0.050f, 0.030f } } },
        { "default_object"_hash,            { { 0.10f, 0.20f, 0.30f }, 0.05f, { 0.100f, 0.050f, 0.030f } } },

        { "materials\\bricks"_hash,         { { 0.03f, 0.04f, 0.07f }, 0.05f, { 0.015f, 0.015f, 0.015f } } },
        { "materials\\concrete"_hash,       { { 0.05f, 0.07f, 0.08f }, 0.05f, { 0.015f, 0.002f, 0.001f } } },
        { "materials\\stucco"_hash,         { { 0.01f, 0.02f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } } },
        { "materials\\flooring_tile"_hash,  { { 0.01f, 0.02f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } } },
        { "gravel"_hash,    { { 0.60f, 0.70f, 0.80f }, 0.05f, { 0.031f, 0.012f, 0.008f } } },
        { "carpet"_hash,    { { 0.24f, 0.69f, 0.73f }, 0.05f, { 0.020f, 0.005f, 0.003f } } },
        { "glass"_hash,     { { 0.06f, 0.03f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } } },
        { "plaster"_hash,   { { 0.12f, 0.06f, 0.04f }, 0.05f, { 0.056f, 0.056f, 0.004f } } },
        { "wood"_hash,      { { 0.11f, 0.07f, 0.06f }, 0.05f, { 0.070f, 0.014f, 0.005f } } },
        { "metal"_hash,     { { 0.20f, 0.07f, 0.06f }, 0.05f, { 0.200f, 0.025f, 0.010f } } },
        { "rock"_hash,      { { 0.13f, 0.20f, 0.24f }, 0.05f, { 0.015f, 0.002f, 0.001f } } },
    };

    acoustics = { { 0.10f, 0.20f, 0.30f }, 0.05f, { 0.100f, 0.050f, 0.030f } };
    return false;
}

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

    if (fs.find_chunk(GAMEMTL_CHUNK_ACOUSTICS))
    {
        fs.r(&Acoustics, sizeof(Acoustics));
        bHasAcousticsParams = true;
    }
    else
    {
        bHasAcousticsParams = assign_default_acoustics(m_Name.c_str());
    }
}

CGameMtlLibrary::CGameMtlLibrary()
{
    material_index = 0;
    material_pair_index = 0;
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
    const u16 version = fs.r_u16();
    if (GAMEMTL_CURRENT_VERSION != version)
    {
        Log("CGameMtlLibrary: invalid version. Library can't load.");
        FS.r_close(F);
        return;
    }

    m_file_age = fs.get_age();

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
            SGameMtl* M = materials.emplace_back(xr_new<SGameMtl>());
            M->Load(*O);
        }
        OBJ->close();
    }

    OBJ = fs.open_chunk(GAMEMTLS_CHUNK_MTLS_PAIR);
    if (OBJ)
    {
        u32 count;
        for (IReader* O = OBJ->open_chunk_iterator(count); O; O = OBJ->open_chunk_iterator(count, O))
        {
            SGameMtlPair* M = material_pairs.emplace_back(xr_new<SGameMtlPair>(this));
            M->Load(*O);
        }
        OBJ->close();
    }
    const u32 mtlCount = materials.size();
    material_pairs_rt.resize(mtlCount * mtlCount, 0);
    for (const auto& mtlPair : material_pairs)
    {
        const int idx0 = GetMaterialIdx(mtlPair->mtl0) * mtlCount + GetMaterialIdx(mtlPair->mtl1);
        const int idx1 = GetMaterialIdx(mtlPair->mtl1) * mtlCount + GetMaterialIdx(mtlPair->mtl0);
        material_pairs_rt[idx0] = mtlPair;
        material_pairs_rt[idx1] = mtlPair;
    }
    FS.r_close(F);
}
