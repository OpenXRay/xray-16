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

bool assign_default_acoustics(const shared_str& material, SGameMtl::MtlAcoustics& acoustics)
{
    using entry_type = std::tuple<shared_str, SGameMtl::MtlAcoustics>;

    static entry_type predefined[] =
    {
        { "default",                       { { 0.10f, 0.20f, 0.30f }, 0.05f, { 0.100f, 0.050f, 0.030f } } },
        { "default_object",                { { 0.10f, 0.20f, 0.30f }, 0.05f, { 0.100f, 0.050f, 0.030f } } },

        { "materials\\asphalt",            { { 0.15f, 0.25f, 0.35f }, 0.1f,  { 0.05f,  0.02f,  0.01f  } } },
        { "materials\\bricks",             { { 0.03f, 0.04f, 0.07f }, 0.05f, { 0.015f, 0.015f, 0.015f } } },
        { "materials\\bush",               { { 0.20f, 0.30f, 0.40f }, 0.2f,  { 0.10f,  0.05f,  0.02f  } } },
        { "materials\\bush_sux",           { { 0.30f, 0.40f, 0.50f }, 0.3f,  { 0.15f,  0.07f,  0.03f  } } },
        { "materials\\cloth",              { { 0.20f, 0.30f, 0.40f }, 0.2f,  { 0.10f,  0.05f,  0.02f  } } },
        { "materials\\concrete",           { { 0.05f, 0.07f, 0.08f }, 0.05f, { 0.015f, 0.002f, 0.001f } } },
        { "materials\\death",              { { 0.00f, 0.00f, 0.00f }, 0.00f, { 1.0f,   1.0f,   1.0f   } } },
        { "materials\\dirt",               { { 0.20f, 0.30f, 0.40f }, 0.15f, { 0.08f,  0.04f,  0.02f  } } },
        { "materials\\earth",              { { 0.25f, 0.35f, 0.45f }, 0.20f, { 0.10f,  0.05f,  0.03f  } } },
        { "materials\\earth_death",        { { 0.25f, 0.35f, 0.45f }, 0.20f, { 0.10f,  0.05f,  0.03f  } } },
        { "materials\\earth_slide",        { { 0.25f, 0.35f, 0.45f }, 0.20f, { 0.10f,  0.05f,  0.03f  } } },
        { "materials\\flooring_tile",      { { 0.01f, 0.02f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } } },
        { "materials\\glass",              { { 0.06f, 0.03f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } } },
        { "materials\\grass",              { { 0.30f, 0.40f, 0.50f }, 0.25f, { 0.12f,  0.06f,  0.04f  } } },
        { "materials\\gravel",             { { 0.60f, 0.70f, 0.80f }, 0.05f, { 0.031f, 0.012f, 0.008f } } },
        { "materials\\metal",              { { 0.20f, 0.07f, 0.06f }, 0.05f, { 0.200f, 0.025f, 0.010f } } },
        { "materials\\metal_pipe",         { { 0.15f, 0.10f, 0.08f }, 0.10f, { 0.10f,  0.050f, 0.020f } } },
        { "materials\\metal_plate",        { { 0.10f, 0.05f, 0.04f }, 0.02f, { 0.30f,  0.030f, 0.015f } } },
        { "materials\\sand",               { { 0.20f, 0.30f, 0.40f }, 0.2f,  { 0.10f,  0.05f,  0.02f  } } },
        { "materials\\setka_rabica",       { { 0.01f, 0.01f, 0.01f }, 0.02f, { 0.5f,   0.5f,   0.5f   } } },
        { "materials\\shifer",             { { 0.02f, 0.04f, 0.06f }, 0.2f,  { 0.05f,  0.02f,  0.01f  } } },
        { "materials\\stucco",             { { 0.01f, 0.02f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } } },
        { "materials\\tin",                { { 0.05f, 0.10f, 0.15f }, 0.2f,  { 0.70f,  0.60f,  0.50f  } } },
        { "materials\\tree_trunk",         { { 0.11f, 0.07f, 0.06f }, 0.05f, { 0.070f, 0.014f, 0.005f } } },
        { "materials\\water",              { { 0.01f, 0.02f, 0.03f }, 0.05f, { 0.99f,  0.98f,  0.97f } } },
        { "materials\\water_radiation",    { { 0.01f, 0.02f, 0.03f }, 0.05f, { 0.99f,  0.98f,  0.97f } } },
        { "materials\\wood",               { { 0.11f, 0.07f, 0.06f }, 0.05f, { 0.070f, 0.014f, 0.005f } } },
        { "materials\\wooden_board",       { { 0.11f, 0.07f, 0.06f }, 0.05f, { 0.070f, 0.014f, 0.005f } } },

        { "objects\\barrel",               { { 0.20f, 0.30f, 0.40f }, 0.2f,  { 0.05f,  0.03f,  0.02f  } } },
        { "objects\\bottle",               { { 0.06f, 0.03f, 0.02f }, 0.05f, { 0.06f,  0.04f,  0.01f  } } },
        { "objects\\bullet",               { { 0.20f, 0.07f, 0.06f }, 0.05f, { 0.200f, 0.025f, 0.010f } } },
        { "objects\\car_cabine",           { { 0.2f,  0.3f,  0.4f  }, 0.2f,  { 0.1f,   0.05f,  0.02f  } } },
        { "objects\\car_wheel",            { { 0.2f,  0.3f,  0.4f  }, 0.2f,  { 0.1f,   0.05f,  0.02f  } } },
        { "objects\\clothes",              { { 0.25f, 0.35f, 0.45f }, 0.3f,  { 0.15f,  0.07f,  0.03f  } } },
        { "objects\\concrete_box",         { { 0.05f, 0.07f, 0.08f }, 0.05f, { 0.015f, 0.002f, 0.001f } } },
        { "objects\\dead_body",            { { 0.10f, 0.20f, 0.30f }, 0.05f, { 0.100f, 0.050f, 0.030f } } },
        { "objects\\fuel_can",             { { 0.3f,  0.4f,  0.5f  }, 0.2f,  { 0.1f,   0.05f,  0.02f  } } },
        { "objects\\glass",                { { 0.06f, 0.03f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } } },
        { "objects\\knife",                { { 0.20f, 0.07f, 0.06f }, 0.05f, { 0.200f, 0.025f, 0.010f } } },
        { "objects\\large_furniture",      { { 0.20f, 0.30f, 0.40f }, 0.2f,  { 0.05f,  0.03f,  0.02f  } } },
        { "objects\\large_metal_trash",    { { 0.20f, 0.07f, 0.06f }, 0.05f, { 0.200f, 0.025f, 0.010f } } },
        { "objects\\large_weapon",         { { 0.20f, 0.07f, 0.06f }, 0.05f, { 0.200f, 0.025f, 0.010f } } },
        { "objects\\metal_box",            { { 0.20f, 0.07f, 0.06f }, 0.05f, { 0.200f, 0.025f, 0.010f } } },
        { "objects\\monster_body",         { { 0.6f,  0.7f,  0.8f  }, 0.3f,  { 0.1f,   0.05f,  0.02f  } } },
        { "objects\\small_box",            { { 0.10f, 0.05f, 0.04f }, 0.02f, { 0.30f,  0.030f, 0.015f } } },
        { "objects\\small_metal_trash",    { { 0.10f, 0.05f, 0.04f }, 0.02f, { 0.30f,  0.030f, 0.015f } } },
        { "objects\\small_weapon",         { { 0.10f, 0.05f, 0.04f }, 0.02f, { 0.30f,  0.030f, 0.015f } } },
        { "objects\\tin_can",              { { 0.05f, 0.10f, 0.15f }, 0.2f,  { 0.70f,  0.60f,  0.50f  } } },
    };

    auto it = std::find_if(std::begin(predefined), std::end(predefined), [&material](const entry_type& entry)
    {
        const auto& [name, _] = entry;
        return name == material;
    });

    bool found = true;
    if (it == std::end(predefined))
    {
        // Assign first material, which is 'default'
        it = std::begin(predefined);
        found = false;
    }

    const auto& [_, entry] = *it;
    acoustics = entry;
    return found;
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
        fs.r(&Acoustics, sizeof(Acoustics));
    else
    {
        const bool predefined_found = assign_default_acoustics(m_Name, Acoustics);
        if (!predefined_found)
        {
            Acoustics.fAbsorption[0] = fSndOcclusionFactor;
            Acoustics.fAbsorption[1] = fSndOcclusionFactor;
            Acoustics.fAbsorption[2] = fSndOcclusionFactor;
        }
    }
}

CGameMtlLibrary::CGameMtlLibrary()
{
    material_index = 0;
    material_pair_index = 0;
}

void CGameMtlLibrary::Load()
{
    ZoneScoped;

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
