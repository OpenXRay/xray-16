#include "StdAfx.h"
#include "WeaponAmmo.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrServer_Objects_ALife_Items.h"
#include "Actor_Flags.h"
#include "Inventory.h"
#include "Weapon.h"
#include "Level_Bullet_Manager.h"
#include "ai_space.h"
#include "xrEngine/GameMtlLib.h"
#include "Level.h"
#include "string_table.h"

#define BULLET_MANAGER_SECTION "bullet_manager"

CCartridge::CCartridge()
{
    m_flags.assign(cfTracer | cfRicochet);
    m_ammoSect = NULL;
    param_s.Init();
    bullet_material_idx = u16(-1);
}

void CCartridge::Load(LPCSTR section, u8 LocalAmmoType)
{
    m_ammoSect = section;
    m_LocalAmmoType = LocalAmmoType;
    param_s.kDist = pSettings->r_float(section, "k_dist");
    param_s.kDisp = pSettings->r_float(section, "k_disp");
    param_s.kHit = pSettings->r_float(section, "k_hit");
    //.	param_s.kCritical			= pSettings->r_float(section, "k_hit_critical");
    param_s.kImpulse = pSettings->r_float(section, "k_impulse");
    // m_kPierce				= pSettings->r_float(section, "k_pierce");
    param_s.kAP = pSettings->r_float(section, "k_ap");
    param_s.u8ColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracer_color_ID", 0);

    if (pSettings->line_exist(section, "k_air_resistance"))
        param_s.kAirRes = pSettings->r_float(section, "k_air_resistance");
    else
        param_s.kAirRes = pSettings->r_float(BULLET_MANAGER_SECTION, "air_resistance_k");

    m_flags.set(cfTracer, pSettings->r_bool(section, "tracer"));
    param_s.buckShot = pSettings->r_s32(section, "buck_shot");
    param_s.impair = pSettings->r_float(section, "impair");
    param_s.fWallmarkSize = pSettings->r_float(section, "wm_size");

    m_flags.set(cfCanBeUnlimited | cfRicochet, TRUE);
    m_flags.set(cfMagneticBeam, FALSE);

    if (pSettings->line_exist(section, "allow_ricochet"))
    {
        if (!pSettings->r_bool(section, "allow_ricochet"))
            m_flags.set(cfRicochet, FALSE);
    }
    if (pSettings->line_exist(section, "magnetic_beam_shot"))
    {
        if (pSettings->r_bool(section, "magnetic_beam_shot"))
            m_flags.set(cfMagneticBeam, TRUE);
    }

    if (pSettings->line_exist(section, "can_be_unlimited"))
        m_flags.set(cfCanBeUnlimited, pSettings->r_bool(section, "can_be_unlimited"));

    m_flags.set(cfExplosive, pSettings->r_bool(section, "explosive"));

    bullet_material_idx = GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
    VERIFY(u16(-1) != bullet_material_idx);
    VERIFY(param_s.fWallmarkSize > 0);

    m_InvShortName = StringTable().translate(pSettings->r_string(section, "inv_name_short"));
}

float CCartridge::Weight() const
{
    auto s = m_ammoSect.c_str();
    float res = 0;
    if (s)
    {
        float box = pSettings->r_float(s, "box_size");
        if (box > 0)
        {
            float w = pSettings->r_float(s, "inv_weight");
            res = w / box;
        }
    }
    return res;
}

CWeaponAmmo::CWeaponAmmo(void) {}
CWeaponAmmo::~CWeaponAmmo(void) {}
void CWeaponAmmo::Load(LPCSTR section)
{
    inherited::Load(section);

    cartridge_param.kDist = pSettings->r_float(section, "k_dist");
    cartridge_param.kDisp = pSettings->r_float(section, "k_disp");
    cartridge_param.kHit = pSettings->r_float(section, "k_hit");
    //.	cartridge_param.kCritical	= pSettings->r_float(section, "k_hit_critical");
    cartridge_param.kImpulse = pSettings->r_float(section, "k_impulse");
    // m_kPierce				= pSettings->r_float(section, "k_pierce");
    cartridge_param.kAP = pSettings->r_float(section, "k_ap");
    cartridge_param.u8ColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracer_color_ID", 0);

    if (pSettings->line_exist(section, "k_air_resistance"))
        cartridge_param.kAirRes = pSettings->r_float(section, "k_air_resistance");
    else
        cartridge_param.kAirRes = pSettings->r_float(BULLET_MANAGER_SECTION, "air_resistance_k");
    m_tracer = !!pSettings->r_bool(section, "tracer");
    cartridge_param.buckShot = pSettings->r_s32(section, "buck_shot");
    cartridge_param.impair = pSettings->r_float(section, "impair");
    cartridge_param.fWallmarkSize = pSettings->r_float(section, "wm_size");
    R_ASSERT(cartridge_param.fWallmarkSize > 0);

    m_boxSize = (u16)pSettings->r_s32(section, "box_size");
    m_boxCurr = m_boxSize;
}

BOOL CWeaponAmmo::net_Spawn(CSE_Abstract* DC)
{
    BOOL bResult = inherited::net_Spawn(DC);
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeItemAmmo* l_pW = smart_cast<CSE_ALifeItemAmmo*>(e);
    m_boxCurr = l_pW->a_elapsed;

    if (m_boxCurr > m_boxSize)
        l_pW->a_elapsed = m_boxCurr = m_boxSize;

    return bResult;
}

void CWeaponAmmo::net_Destroy() { inherited::net_Destroy(); }
void CWeaponAmmo::OnH_B_Chield() { inherited::OnH_B_Chield(); }
void CWeaponAmmo::OnH_B_Independent(bool just_before_destroy)
{
    if (!Useful())
    {
        if (Local())
        {
            DestroyObject();
        }
        m_ready_to_destroy = true;
    }
    inherited::OnH_B_Independent(just_before_destroy);
}

bool CWeaponAmmo::Useful() const
{
    // Если IItem еще не полностью использованый, вернуть true
    return !!m_boxCurr;
}
/*
s32 CWeaponAmmo::Sort(PIItem pIItem)
{
    // Если нужно разместить IItem после this - вернуть 1, если
    // перед - -1. Если пофиг то 0.
    CWeaponAmmo *l_pA = smart_cast<CWeaponAmmo*>(pIItem);
    if(!l_pA) return 0;
    if(xr_strcmp(cNameSect(), l_pA->cNameSect())) return 0;
    if(m_boxCurr <= l_pA->m_boxCurr) return 1;
    else return -1;
}
*/
bool CWeaponAmmo::Get(CCartridge& cartridge)
{
    if (!m_boxCurr)
        return false;
    cartridge.m_ammoSect = cNameSect();

    cartridge.param_s = cartridge_param;

    cartridge.m_flags.set(CCartridge::cfTracer, m_tracer);
    cartridge.bullet_material_idx = GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
    cartridge.m_InvShortName = NameShort();
    --m_boxCurr;
    if (m_pInventory)
        m_pInventory->InvalidateState();
    return true;
}

void CWeaponAmmo::renderable_Render()
{
    if (!m_ready_to_destroy)
        inherited::renderable_Render();
}

void CWeaponAmmo::UpdateCL()
{
    VERIFY2(_valid(renderable.xform), *cName());
    inherited::UpdateCL();
    VERIFY2(_valid(renderable.xform), *cName());

    if (!IsGameTypeSingle())
        make_Interpolation();

    VERIFY2(_valid(renderable.xform), *cName());
}

void CWeaponAmmo::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);

    P.w_u16(m_boxCurr);
}

void CWeaponAmmo::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);

    P.r_u16(m_boxCurr);
}

CInventoryItem* CWeaponAmmo::can_make_killing(const CInventory* inventory) const
{
    VERIFY(inventory);

    TIItemContainer::const_iterator I = inventory->m_all.begin();
    TIItemContainer::const_iterator E = inventory->m_all.end();
    for (; I != E; ++I)
    {
        CWeapon* weapon = smart_cast<CWeapon*>(*I);
        if (!weapon)
            continue;
        xr_vector<shared_str>::const_iterator i =
            std::find(weapon->m_ammoTypes.begin(), weapon->m_ammoTypes.end(), cNameSect());
        if (i != weapon->m_ammoTypes.end())
            return (weapon);
    }

    return (0);
}

float CWeaponAmmo::Weight() const
{
    if (m_boxSize > 0)
    {
        float res = inherited::Weight();
        res *= (float)m_boxCurr / (float)m_boxSize;
        return res;
    }
    return 0.f;
}

u32 CWeaponAmmo::Cost() const
{
    u32 res = inherited::Cost();

    res = iFloor(res * (float)m_boxCurr / (float)m_boxSize + 0.5f);

    return res;
}
