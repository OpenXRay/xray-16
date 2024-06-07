#pragma once

#include "xrCore/xrstring.h"
#include "xrCore/_flags.h"
#include "xrCommon/xr_vector.h"

#include "xrSound/Sound.h"
#include "Include/xrRender/WallMarkArray.h"
#include "Include/xrRender/RenderFactory.h"

#ifdef XRMTL_LIB_EXPORTS
#define MTL_EXPORT_API XR_EXPORT
#else
#define MTL_EXPORT_API XR_IMPORT
#endif

constexpr u16 GAMEMTL_CURRENT_VERSION     = 0x0001;

constexpr u32 GAMEMTLS_CHUNK_VERSION      = 0x1000;
constexpr u32 GAMEMTLS_CHUNK_AUTOINC      = 0x1001;
constexpr u32 GAMEMTLS_CHUNK_MTLS         = 0x1002;
constexpr u32 GAMEMTLS_CHUNK_MTLS_PAIR    = 0x1003;

constexpr u32 GAMEMTL_CHUNK_MAIN          = 0x1000;
constexpr u32 GAMEMTL_CHUNK_FLAGS         = 0x1001;
constexpr u32 GAMEMTL_CHUNK_PHYSICS       = 0x1002;
constexpr u32 GAMEMTL_CHUNK_FACTORS       = 0x1003;
constexpr u32 GAMEMTL_CHUNK_FLOTATION     = 0x1004;
constexpr u32 GAMEMTL_CHUNK_DESC          = 0x1005;
constexpr u32 GAMEMTL_CHUNK_INJURIOUS     = 0x1006;
constexpr u32 GAMEMTL_CHUNK_DENSITY       = 0x1007;
constexpr u32 GAMEMTL_CHUNK_FACTORS_MP    = 0x1008;
constexpr u32 GAMEMTL_CHUNK_ACOUSTICS     = 0x1009;

constexpr u32 GAMEMTLPAIR_CHUNK_PAIR      = 0x1000;
//constexpr u32 GAMEMTLPAIR_CHUNK_FLOTATION = 0x1001; // obsolete
constexpr u32 GAMEMTLPAIR_CHUNK_BREAKING  = 0x1002;
constexpr u32 GAMEMTLPAIR_CHUNK_STEP      = 0x1003;
//constexpr u32 GAMEMTLPAIR_CHUNK_COLLIDE   = 0x1004; // obsolete / rename HIT
constexpr u32 GAMEMTLPAIR_CHUNK_COLLIDE   = 0x1005;

constexpr int GAMEMTL_SUBITEM_COUNT       = 4;

constexpr u32 GAMEMTL_NONE_ID             = u32(-1);
constexpr u32 GAMEMTL_NONE_IDX            = u16(-1);

constexpr pcstr GAMEMTL_FILENAME          = "gamemtl.xr";

// fwd. decl.
class IReader;
class IWriter;

struct MTL_EXPORT_API SGameMtl
{
    friend class CGameMtlLibrary;

protected:
    int ID; // auto number
public:
    enum : u32
    {
        flBreakable = (1ul << 0ul),
        // flShootable = (1ul<<1ul),
        flBounceable = (1ul << 2ul),
        flSkidmark = (1ul << 3ul),
        flBloodmark = (1ul << 4ul),
        flClimable = (1ul << 5ul),
        // flWalkOn = (1ul<<6ul), // obsolette
        flPassable = (1ul << 7ul),
        flDynamic = (1ul << 8ul),
        flLiquid = (1ul << 9ul),
        flSuppressShadows = (1ul << 10ul),
        flSuppressWallmarks = (1ul << 11ul),
        flActorObstacle = (1ul << 12ul),
        flNoRicoshet = (1ul << 13ul),
        flInjurious = (1ul << 28ul), // flInjurious = fInjuriousSpeed > 0.f
        flShootable = (1ul << 29ul),
        flTransparent = (1ul << 30ul),
        flSlowDown = (1ul << 31ul) // flSlowDown = (fFlotationFactor<1.f)
    };

public:
    shared_str m_Name;
    shared_str m_Desc;
    Flags32 Flags;
    // physics part
    float fPHFriction; // ?
    float fPHDamping; // ?
    float fPHSpring; // ?
    float fPHBounceStartVelocity; // ?
    float fPHBouncing; // ?
    // shoot&bounce&visibility&flotation
    float fFlotationFactor; // 0.f - 1.f (1.f-полностью проходимый)
    float fShootFactor; // 0.f - 1.f (1.f-полностью простреливаемый)
    float fShootFactorMP; // 0.f - 1.f (1.f-полностью простреливаемый)
    float fBounceDamageFactor; // 0.f - 100.f
    float fInjuriousSpeed; // 0.f - ... (0.f-не отбирает здоровье (скорость уменьшения здоровья))
    float fVisTransparencyFactor; // 0.f - 1.f (1.f-полностью прозрачный)
    float fSndOcclusionFactor; // 0.f - 1.f (1.f-полностью слышен)
    float fDensityFactor;

    struct MtlAcoustics
    {
        float fAbsorption[3];
        float fScattering;
        float fTransmission[3];
    };
    MtlAcoustics Acoustics{};

public:
    SGameMtl()
    {
        ID = -1;
        m_Name = "unknown";
        Flags.zero();
        // factors
        fFlotationFactor = 1.f;
        fShootFactor = 0.f;
        fShootFactorMP = 0.f;
        fBounceDamageFactor = 1.f;
        fInjuriousSpeed = 0.f;
        fVisTransparencyFactor = 0.f;
        fSndOcclusionFactor = 0.f;
        // physics
        fPHFriction = 1.f;
        fPHDamping = 1.f;
        fPHSpring = 1.f;
        fPHBounceStartVelocity = 0.f;
        fPHBouncing = 0.1f;
        fDensityFactor = 0.0f;
    }
    void Load(IReader& fs);
    void Save(IWriter& fs);
    int GetID() const { return ID; }
#ifdef _EDITOR
    void FillProp(PropItemVec& values, ListItem* owner);
#endif
};

struct MTL_EXPORT_API SGameMtlPair
{
    friend class CGameMtlLibrary;
    enum
    {
        // flFlotation = (1<<0),
        flBreakingSounds = (1 << 1),
        flStepSounds = (1 << 2),
        // flCollideSounds = (1<<3),
        flCollideSounds = (1 << 4),
        flCollideParticles = (1 << 5),
        flCollideMarks = (1 << 6)
    };
    CGameMtlLibrary* m_Owner;

private:
    int mtl0;
    int mtl1;

protected:
    int ID; // auto number
    int ID_parent;

public:
    Flags32 OwnProps;

public:
    xr_vector<ref_sound> BreakingSounds;
    xr_vector<ref_sound> StepSounds;
    xr_vector<ref_sound> CollideSounds;
    xr_vector<shared_str> CollideParticles;
    FactoryPtr<IWallMarkArray> CollideMarks;

public:
    SGameMtlPair(CGameMtlLibrary* owner)
    {
        mtl0 = -1;
        mtl1 = -1;
        ID = -1;
        ID_parent = -1;
        m_Owner = owner;
        OwnProps.one();
    }
    ~SGameMtlPair();

    int GetMtl0() const { return mtl0; }
    int GetMtl1() const { return mtl1; }
    int GetID() const { return ID; }
    void SetPair(int m0, int m1)
    {
        mtl0 = m0;
        mtl1 = m1;
    }
    bool IsPair(int m0, int m1) const
    {
        return (mtl0 == m0 && mtl1 == m1) || (mtl0 == m1 && mtl1 == m0);
    }
    int GetParent() const { return ID_parent; }
    void Save(IWriter& fs);
    void Load(IReader& fs);
#ifdef DEBUG
    const char* dbg_Name() const;
#endif

#ifdef _EDITOR
    PropValue* propBreakingSounds;
    PropValue* propStepSounds;
    PropValue* propCollideSounds;
    PropValue* propCollideParticles;
    PropValue* propCollideMarks;

    SGameMtlPair(const SGameMtlPair& src);
    void OnFlagChange(PropValue* sender);
    void OnParentClick(ButtonValue* sender, bool& bModif, bool& bSafe);
    void OnCommandClick(ButtonValue* sender, bool& bModif, bool& bSafe);
    void FillChooseMtl(ChooseItemVec& items, void* param);
    void FillProp(PropItemVec& values);
    void TransferFromParent(SGameMtlPair* parent);
    bool SetParent(int parentId);
#endif
};

using GameMtlPairVec = xr_vector<SGameMtlPair*>;
using GameMtlPairIt = GameMtlPairVec::iterator;

class MTL_EXPORT_API CGameMtlLibrary
{
private:
    int material_index;
    int material_pair_index;

    xr_vector<SGameMtl*> materials;
    GameMtlPairVec material_pairs;
    GameMtlPairVec material_pairs_rt;

    u32 m_file_age{};

public:
    CGameMtlLibrary();
    ~CGameMtlLibrary() {}
    void Unload()
    {
        material_pairs_rt.clear();
        for (auto& mtl : materials)
            xr_delete(mtl);
        materials.clear();
        for (auto& mtlPair : material_pairs)
            xr_delete(mtlPair);
        material_pairs.clear();
    }

    auto GetMaterialIt(pcstr name)
    {
        auto pred = [&](const SGameMtl* mtl) { return !xr_strcmpi(mtl->m_Name.c_str(), name); };
        return std::find_if(materials.begin(), materials.end(), pred);
    }
    auto GetMaterialIt(const shared_str& name)
    {
        auto pred = [&](const SGameMtl* mtl) { return mtl->m_Name.equal(name); };
        return std::find_if(materials.begin(), materials.end(), pred);
    }
    auto GetMaterialItByID(int id)
    {
        auto pred = [&](const SGameMtl* mtl) { return mtl->ID == id; };
        return std::find_if(materials.begin(), materials.end(), pred);
    }
    u32 GetMaterialID(pcstr name)
    {
        const auto it = GetMaterialIt(name);
        return it == materials.end() ? GAMEMTL_NONE_ID : (*it)->ID;
    }
    SGameMtl* GetMaterial(pcstr name)
    {
        const auto it = GetMaterialIt(name);
        return materials.end() != it ? *it : nullptr;
    }
    SGameMtl* GetMaterialByID(s32 id)
    {
        const auto it = GetMaterialItByID(id);
        return it != materials.end() ? *it : nullptr;
    }
    u16 GetMaterialIdx(int ID)
    {
        const auto it = GetMaterialItByID(ID);
        VERIFY(materials.end() != it);
        return u16(it - materials.begin());
    }
    u16 GetMaterialIdx(pcstr name)
    {
        const auto it = GetMaterialIt(name);
        VERIFY(materials.end() != it);
        return u16(it - materials.begin());
    }
    SGameMtl* GetMaterialByIdx(u16 idx) const
    {
        VERIFY(idx < (u16)materials.size());
        return materials[idx];
    }

    auto FirstMaterial() { return materials.begin(); }
    auto LastMaterial() { return materials.end(); }

    const auto& Materials() const { return materials; }

    u32 CountMaterial() const { return materials.size(); }

#ifdef _EDITOR
    SGameMtl* AppendMaterial(SGameMtl* parent);
    void RemoveMaterial(pcstr name);
    void CopyMtlPairs(SGameMtl* src, SGameMtl* dst);
    bool UpdateMtlPairs(SGameMtl* src);
    bool UpdateMtlPairs();
    pcstr MtlPairToName(int mtl0, int mtl1);
    void NameToMtlPair(pcstr name, int& mtl0, int& mtl1);
    void MtlNameToMtlPair(pcstr name, int& mtl0, int& mtl1);
    SGameMtlPair* CreateMaterialPair(int m0, int m1, SGameMtlPair* parent = nullptr);
    SGameMtlPair* AppendMaterialPair(int m0, int m1, SGameMtlPair* parent = nullptr);
    void RemoveMaterialPair(pcstr name);
    void RemoveMaterialPair(GameMtlPairIt rem_it);
    void RemoveMaterialPair(int mtl);
    void RemoveMaterialPair(int mtl0, int mtl1);
    GameMtlPairIt GetMaterialPairIt(int id);
    GameMtlPairIt GetMaterialPairIt(int mtl0, int mtl1);
    SGameMtlPair* GetMaterialPair(int id);
    SGameMtlPair* GetMaterialPair(int mtl0, int mtl1);
    SGameMtlPair* GetMaterialPair(const char* name);
#endif // _EDITOR

    // game
    SGameMtlPair* GetMaterialPairByIndices(u16 i0, u16 i1) const
    {
        const u32 mtlCount = materials.size();
        R_ASSERT(i0 < mtlCount && i1 < mtlCount);
        return material_pairs_rt[i1 * mtlCount + i0];
    }

    GameMtlPairIt FirstMaterialPair() { return material_pairs.begin(); }
    GameMtlPairIt LastMaterialPair() { return material_pairs.end(); }

    // IO routines
    void Load();
    bool Save();

    [[nodiscard]]
    auto GetFileAge() const { return m_file_age; }
};

extern MTL_EXPORT_API CGameMtlLibrary GMLib;
