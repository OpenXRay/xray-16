#include "pch_script.h"

#include "WeaponMagazined.h"
#include "Actor.h"
#include "ParticlesObject.h"
#include "Scope.h"
#include "Silencer.h"
#include "GrenadeLauncher.h"
#include "Inventory.h"
#include "InventoryOwner.h"
#include "xrServer_Objects_ALife_Items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "UIGameCustom.h"
#include "Common/object_broker.h"
#include "string_table.h"
#include "MPPlayersBag.h"
#include "ui/UIXmlInit.h"
#include "xrUICore/Static/UIStatic.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "HudSound.h"

CUIXml* pWpnScopeXml = nullptr;

void createWpnScopeXML()
{
    if (!pWpnScopeXml)
    {
        pWpnScopeXml = new CUIXml();
        pWpnScopeXml->Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "scopes.xml");
    }
}

CWeaponMagazined::CWeaponMagazined(ESoundTypes eSoundType) : CWeapon()
{
    m_eSoundShow = ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
    m_eSoundHide = ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
    m_eSoundShot = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
    m_eSoundEmptyClick = ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
    m_eSoundReload = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
#ifdef NEW_SOUNDS
    m_eSoundReloadEmpty = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
#endif
    m_sounds_enabled = true;

    m_sSndShotCurrent = nullptr;
    m_sSilencerFlameParticles = m_sSilencerSmokeParticles = nullptr;

    m_bFireSingleShot = false;
    m_iShotNum = 0;
    m_fOldBulletSpeed = 0;
    m_iQueueSize = WEAPON_ININITE_QUEUE;
    m_bLockType = false;
}

CWeaponMagazined::~CWeaponMagazined()
{
    // sounds
}

void CWeaponMagazined::net_Destroy() { inherited::net_Destroy(); }

//AVO: for custom added sounds check if sound exists
bool CWeaponMagazined::WeaponSoundExist(pcstr section, pcstr sound_name) const
{
    pcstr str;
    bool sec_exist = process_if_exists_set(section, sound_name, &CInifile::r_string, str, true);
    if (sec_exist)
        return true;
#ifdef DEBUG
    Msg("~ [WARNING] ------ Sound [%s] does not exist in [%s]", sound_name, section);
#endif
    return false;
}

//-AVO

void CWeaponMagazined::Load(LPCSTR section)
{
    inherited::Load(section);

    // Sounds
    m_sounds.LoadSound(section, "snd_draw", "sndShow", false, m_eSoundShow);
    m_sounds.LoadSound(section, "snd_holster", "sndHide", false, m_eSoundHide);

    //Alundaio: LAYERED_SND_SHOOT
#ifdef LAYERED_SND_SHOOT
    m_layered_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
#else
    m_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
#endif
    //-Alundaio

    m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", false, m_eSoundEmptyClick);
    m_sounds.LoadSound(section, "snd_reload", "sndReload", true, m_eSoundReload);

#ifdef NEW_SOUNDS //AVO: custom sounds go here
    if (WeaponSoundExist(section, "snd_reload_empty"))
        m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", true, m_eSoundReloadEmpty);
    if (WeaponSoundExist(section, "snd_reload_misfire"))
        m_sounds.LoadSound(section, "snd_reload_misfire", "sndReloadMisfire", true, m_eSoundReloadMisfire);
#endif //-NEW_SOUNDS

    m_sSndShotCurrent = "sndShot";

    //звуки и партиклы глушителя, если такой есть
    if (m_eSilencerStatus == ALife::eAddonAttachable || m_eSilencerStatus == ALife::eAddonPermanent)
    {
        if (pSettings->line_exist(section, "silencer_flame_particles"))
            m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
        if (pSettings->line_exist(section, "silencer_smoke_particles"))
            m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");

        //Alundaio: LAYERED_SND_SHOOT Silencer
#ifdef LAYERED_SND_SHOOT
        m_layered_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
        if (WeaponSoundExist(section, "snd_silncer_shot_actor"))
            m_layered_sounds.LoadSound(section, "snd_silncer_shot_actor", "sndSilencerShotActor", false, m_eSoundShot);
#else
        m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
        if (WeaponSoundExist(section, "snd_silncer_shot_actor"))
            m_sounds.LoadSound(section, "snd_silncer_shot_actor", "sndSilencerShotActor", false, m_eSoundShot);
#endif
        //-Alundaio
    }

    m_iBaseDispersionedBulletsCount = READ_IF_EXISTS(pSettings, r_u8, section, "base_dispersioned_bullets_count", 0);
    m_fBaseDispersionedBulletsSpeed =
        READ_IF_EXISTS(pSettings, r_float, section, "base_dispersioned_bullets_speed", m_fStartBulletSpeed);

    if (pSettings->line_exist(section, "fire_modes"))
    {
        m_bHasDifferentFireModes = true;
        shared_str FireModesList = pSettings->r_string(section, "fire_modes");
        int ModesCount = _GetItemCount(FireModesList.c_str());
        m_aFireModes.clear();

        for (int i = 0; i < ModesCount; i++)
        {
            string16 sItem;
            _GetItem(FireModesList.c_str(), i, sItem);
            m_aFireModes.push_back((s8)atoi(sItem));
        }

        m_iCurFireMode = ModesCount - 1;
        m_iPrefferedFireMode = READ_IF_EXISTS(pSettings, r_s16, section, "preffered_fire_mode", -1);
    }
    else
    {
        m_bHasDifferentFireModes = false;
    }
    LoadSilencerKoeffs();
}

void CWeaponMagazined::FireStart()
{
    if (!IsMisfire())
    {
        if (IsValid())
        {
            if (!IsWorking() || AllowFireWhileWorking())
            {
                if (GetState() == eReload)
                    return;
                if (GetState() == eShowing)
                    return;
                if (GetState() == eHiding)
                    return;
                if (GetState() == eMisfire)
                    return;

                inherited::FireStart();

                if (iAmmoElapsed == 0)
                    OnMagazineEmpty();
                else
                {
                    R_ASSERT(H_Parent());
                    SwitchState(eFire);
                }
            }
        }
        else
        {
            if (eReload != GetState())
                OnMagazineEmpty();
        }
    }
    else
    { // misfire
        //Alundaio
#ifdef EXTENDED_WEAPON_CALLBACKS
        CGameObject *object = smart_cast<CGameObject*>(H_Parent());
        if (object)
            object->callback(GameObject::eOnWeaponJammed)(object->lua_game_object(), this->lua_game_object());
#endif
        //-Alundaio

        if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
            CurrentGameUI()->AddCustomStatic("gun_jammed", true);

        OnEmptyClick();
    }
}

void CWeaponMagazined::FireEnd()
{
    inherited::FireEnd();

    // XXX: disable autoreload via console
    CActor* actor = smart_cast<CActor*>(H_Parent());
    if (m_pInventory && !iAmmoElapsed && actor && GetState() != eReload)
        Reload();
}

void CWeaponMagazined::Reload()
{
    inherited::Reload();
    TryReload();
}

bool CWeaponMagazined::TryReload()
{
    if (m_pInventory)
    {
        if (IsGameTypeSingle() && ParentIsActor())
        {
            int AC = GetSuitableAmmoTotal();
            Actor()->callback(GameObject::eWeaponNoAmmoAvailable)(lua_game_object(), AC);
        }

        m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str()));

        if (IsMisfire() && iAmmoElapsed)
        {
            SetPending(true);
            SwitchState(eReload);
            return true;
        }

        if (m_pCurrentAmmo || unlimited_ammo())
        {
            SetPending(true);
            SwitchState(eReload);
            return true;
        }
        else
            for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
            {
                m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
                if (m_pCurrentAmmo)
                {
                    m_set_next_ammoType_on_reload = i;
                    SetPending(true);
                    SwitchState(eReload);
                    return true;
                }
            }
    }

    if (GetState() != eIdle)
        SwitchState(eIdle);

    return false;
}

bool CWeaponMagazined::IsAmmoAvailable()
{
    if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str())))
        return (true);
    else
        for (u32 i = 0; i < m_ammoTypes.size(); ++i)
            if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str())))
                return (true);
    return (false);
}

void CWeaponMagazined::OnMagazineEmpty()
{
    if (GetState() == eIdle)
    {
        OnEmptyClick();
        return;
    }

    if (GetNextState() != eMagEmpty && GetNextState() != eReload)
    {
        SwitchState(eMagEmpty);
    }

    inherited::OnMagazineEmpty();
}

void CWeaponMagazined::UnloadMagazine(bool spawn_ammo)
{
    xr_map<LPCSTR, u16> l_ammo;

    while (!m_magazine.empty())
    {
        CCartridge& l_cartridge = m_magazine.back();
        xr_map<LPCSTR, u16>::iterator l_it;
        for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
        {
            if (!xr_strcmp(*l_cartridge.m_ammoSect, l_it->first))
            {
                ++(l_it->second);
                break;
            }
        }

        if (l_it == l_ammo.end())
            l_ammo[*l_cartridge.m_ammoSect] = 1;
        m_magazine.pop_back();
        --iAmmoElapsed;
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    if (!spawn_ammo)
        return;

    xr_map<LPCSTR, u16>::iterator l_it;
    for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
    {
        if (m_pInventory)
        {
            CWeaponAmmo* l_pA = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(l_it->first));
            if (l_pA)
            {
                u16 l_free = l_pA->m_boxSize - l_pA->m_boxCurr;
                l_pA->m_boxCurr = l_pA->m_boxCurr + (l_free < l_it->second ? l_free : l_it->second);
                l_it->second = l_it->second - (l_free < l_it->second ? l_free : l_it->second);
            }
        }
        if (l_it->second && !unlimited_ammo())
            SpawnAmmo(l_it->second, l_it->first);
    }
}

void CWeaponMagazined::ReloadMagazine()
{
    m_BriefInfo_CalcFrame = 0;

    //устранить осечку при перезарядке
    if (IsMisfire())
        bMisfire = false;

    if (!m_bLockType)
    {
        m_pCurrentAmmo = nullptr;
    }

    if (!m_pInventory)
        return;

    if (m_set_next_ammoType_on_reload != undefined_ammo_type)
    {
        m_ammoType = m_set_next_ammoType_on_reload;
        m_set_next_ammoType_on_reload = undefined_ammo_type;
    }

    if (!unlimited_ammo())
    {
        if (m_ammoTypes.size() <= m_ammoType)
            return;

        LPCSTR tmp_sect_name = m_ammoTypes[m_ammoType].c_str();

        if (!tmp_sect_name)
            return;

        //попытаться найти в инвентаре патроны текущего типа
        m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(tmp_sect_name));

        if (!m_pCurrentAmmo && !m_bLockType)
        {
            for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
            {
                //проверить патроны всех подходящих типов
                m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
                if (m_pCurrentAmmo)
                {
                    m_ammoType = i;
                    break;
                }
            }
        }
    }

    //нет патронов для перезарядки
    if (!m_pCurrentAmmo && !unlimited_ammo())
        return;

    //разрядить магазин, если загружаем патронами другого типа
    if (!m_bLockType && !m_magazine.empty() &&
        (!m_pCurrentAmmo || xr_strcmp(m_pCurrentAmmo->cNameSect(), *m_magazine.back().m_ammoSect)))
        UnloadMagazine();

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
        m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
    CCartridge l_cartridge = m_DefaultCartridge;
    while (iAmmoElapsed < iMagazineSize)
    {
        if (!unlimited_ammo())
        {
            if (!m_pCurrentAmmo->Get(l_cartridge))
                break;
        }
        ++iAmmoElapsed;
        l_cartridge.m_LocalAmmoType = m_ammoType;
        m_magazine.push_back(l_cartridge);
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    //выкинуть коробку патронов, если она пустая
    if (m_pCurrentAmmo && !m_pCurrentAmmo->m_boxCurr && OnServer())
        m_pCurrentAmmo->SetDropManual(true);

    if (iMagazineSize > iAmmoElapsed)
    {
        m_bLockType = true;
        ReloadMagazine();
        m_bLockType = false;
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeaponMagazined::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);
    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(this->H_Parent());
    switch (S)
    {
    case eIdle: switch2_Idle(); break;
    case eFire: switch2_Fire(); break;
    case eMisfire:
        if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
            CurrentGameUI()->AddCustomStatic("gun_jammed", true);
        break;
    case eMagEmpty: switch2_Empty(); break;
    case eReload:
        if (owner)
            m_sounds_enabled = owner->CanPlayShHdRldSounds();
        switch2_Reload();
        break;
    case eShowing:
        if (owner)
            m_sounds_enabled = owner->CanPlayShHdRldSounds();
        switch2_Showing();
        break;
    case eHiding:
        if (owner)
            m_sounds_enabled = owner->CanPlayShHdRldSounds();
        if (oldState != eHiding)
            switch2_Hiding();
        break;
    case eHidden: switch2_Hidden(); break;
    }
}

void CWeaponMagazined::UpdateCL()
{
    inherited::UpdateCL();
    float dt = Device.fTimeDelta;

    //когда происходит апдейт состояния оружия
    //ничего другого не делать
    if (GetNextState() == GetState())
    {
        switch (GetState())
        {
        case eShowing:
        case eHiding:
        case eReload:
        case eIdle:
        {
            fShotTimeCounter -= dt;
            clamp(fShotTimeCounter, 0.0f, flt_max);
        }
        break;
        case eFire: { state_Fire(dt);
        }
        break;
        case eMisfire: state_Misfire(dt); break;
        case eMagEmpty: state_MagEmpty(dt); break;
        case eHidden: break;
        }
    }

    UpdateSounds();
}

void CWeaponMagazined::UpdateSounds()
{
    if (Device.dwFrame == dwUpdateSounds_Frame)
        return;

    dwUpdateSounds_Frame = Device.dwFrame;

    Fvector P = get_LastFP();
    m_sounds.SetPosition("sndShow", P);
    m_sounds.SetPosition("sndHide", P);
    //. nah	m_sounds.SetPosition("sndShot", P);
    m_sounds.SetPosition("sndReload", P);

#ifdef NEW_SOUNDS //AVO: custom sounds go here
    if (m_sounds.FindSoundItem("sndReloadEmpty", false))
        m_sounds.SetPosition("sndReloadEmpty", P);
#endif //-NEW_SOUNDS

    //. nah	m_sounds.SetPosition("sndEmptyClick", P);
}

void CWeaponMagazined::state_Fire(float dt)
{
    if (iAmmoElapsed > 0)
    {
        VERIFY(fOneShotTime > 0.f);

        Fvector p1, d;
        p1.set(get_LastFP());
        d.set(get_LastFD());

        if (!H_Parent())
            return;
        if (smart_cast<CMPPlayersBag*>(H_Parent()) != nullptr)
        {
            Msg("! WARNING: state_Fire of object [%d][%s] while parent is CMPPlayerBag...", ID(), cNameSect().c_str());
            return;
        }

        CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
        if (nullptr == io->inventory().ActiveItem())
        {
            Log("current_state", GetState());
            Log("next_state", GetNextState());
            Log("item_sect", cNameSect().c_str());
            Log("H_Parent", H_Parent()->cNameSect().c_str());
            StopShooting();
            return; //Alundaio: This is not supposed to happen but it does. GSC was aware but why no return here? Known to cause crash on game load if NPC immediately enters combat.
        }

        CEntity* E = smart_cast<CEntity*>(H_Parent());
        E->g_fireParams(this, p1, d);

        if (!E->g_stateFire())
            StopShooting();

        if (m_iShotNum == 0)
        {
            m_vStartPos = p1;
            m_vStartDir = d;
        };

        VERIFY(!m_magazine.empty());

        while (!m_magazine.empty() && fShotTimeCounter < 0 && (IsWorking() || m_bFireSingleShot) &&
            (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
        {
            if (CheckForMisfire())
            {
                StopShooting();
                return;
            }

            m_bFireSingleShot = false;

            //Alundaio: Use fModeShotTime instead of fOneShotTime if current fire mode is 2-shot burst
            //Alundaio: Cycle down RPM after two shots; used for Abakan/AN-94
            if (GetCurrentFireMode() == 2 || (cycleDown == true && m_iShotNum <= 1))
                fShotTimeCounter = modeShotTime;
            else
                fShotTimeCounter = fOneShotTime;
            //Alundaio: END

            ++m_iShotNum;

            OnShot();

            if (m_iShotNum > m_iBaseDispersionedBulletsCount)
                FireTrace(p1, d);
            else
                FireTrace(m_vStartPos, m_vStartDir);
        }

        if (m_iShotNum == m_iQueueSize)
            m_bStopedAfterQueueFired = true;

        UpdateSounds();
    }

    if (fShotTimeCounter < 0)
    {
        /*
                if(bDebug && H_Parent() && (H_Parent()->ID() != Actor()->ID()))
                {
                    Msg("stop shooting w=[%s] magsize=[%d] sshot=[%s] qsize=[%d] shotnum=[%d]",
                            IsWorking()?"true":"false",
                            m_magazine.size(),
                            m_bFireSingleShot?"true":"false",
                            m_iQueueSize,
                            m_iShotNum);
                }
        */
        if (iAmmoElapsed == 0)
            OnMagazineEmpty();

        StopShooting();
    }
    else
    {
        fShotTimeCounter -= dt;
    }
}

void CWeaponMagazined::state_Misfire(float dt)
{
    OnEmptyClick();
    SwitchState(eIdle);

    bMisfire = true;

    UpdateSounds();
}

void CWeaponMagazined::state_MagEmpty(float dt) {}
void CWeaponMagazined::SetDefaults() { CWeapon::SetDefaults(); }
void CWeaponMagazined::OnShot()
{
    // Sound
    //Alundaio: LAYERED_SND_SHOOT
#ifdef LAYERED_SND_SHOOT
    m_layered_sounds.PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), H_Root(), !!GetHUDmode(), false, (u8)-1);
#else
    PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), (u8)-1); //Alundaio: Play sound at index (ie. snd_shoot, snd_shoot1, snd_shoot2, snd_shoot3)
#endif
    //-Alundaio

    // Camera
    AddShotEffector();

    // Animation
    PlayAnimShoot();

    // Shell Drop
    Fvector vel;
    PHGetLinearVell(vel);
    OnShellDrop(get_LastSP(), vel);

    // Огонь из ствола
    StartFlameParticles();

    //дым из ствола
    ForceUpdateFireParticles();
    StartSmokeParticles(get_LastFP(), vel);
}

void CWeaponMagazined::OnEmptyClick() { PlaySound("sndEmptyClick", get_LastFP()); }
void CWeaponMagazined::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eReload:
        ReloadMagazine();
        SwitchState(eIdle);
        break; // End of reload animation
    case eHiding:
        SwitchState(eHidden);
        break; // End of Hide
    case eShowing:
        SwitchState(eIdle);
        break; // End of Show
    case eIdle:
        switch2_Idle();
        break; // Keep showing idle
    }
    inherited::OnAnimationEnd(state);
}

void CWeaponMagazined::switch2_Idle()
{
    m_iShotNum = 0;
    if (m_fOldBulletSpeed != 0.f)
        SetBulletSpeed(m_fOldBulletSpeed);

    SetPending(false);
    PlayAnimIdle();
}

#ifdef DEBUG
#include "ai/stalker/ai_stalker.h"
#include "object_handler_planner.h"
#endif
void CWeaponMagazined::switch2_Fire()
{
    CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
    CInventoryItem* ii = smart_cast<CInventoryItem*>(this);
#ifdef DEBUG
    if (!io)
        return;
    // VERIFY2					(io,make_string("no inventory owner, item %s",*cName()));

    if (ii != io->inventory().ActiveItem())
        Msg("! not an active item, item %s, owner %s, active item %s", *cName(), *H_Parent()->cName(),
            io->inventory().ActiveItem() ? *io->inventory().ActiveItem()->object().cName() : "no_active_item");

    if (!(io && (ii == io->inventory().ActiveItem())))
    {
        CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(H_Parent());
        if (stalker)
        {
            stalker->planner().show();
            stalker->planner().show_current_world_state();
            stalker->planner().show_target_world_state();
        }
    }
#else
    if (!io)
        return;
#endif // DEBUG

    //
    //	VERIFY2(
    //		io && (ii == io->inventory().ActiveItem()),
    //		make_string(
    //			"item[%s], parent[%s]",
    //			*cName(),
    //			H_Parent() ? *H_Parent()->cName() : "no_parent"
    //		)
    //	);

    m_bStopedAfterQueueFired = false;
    m_bFireSingleShot = true;
    m_iShotNum = 0;

    if ((OnClient() || Level().IsDemoPlay()) && !IsWorking())
        FireStart();
}

void CWeaponMagazined::switch2_Empty()
{
    OnZoomOut();

    if (!TryReload())
    {
        OnEmptyClick();
    }
    else
    {
        inherited::FireEnd();
    }
}
void CWeaponMagazined::PlayReloadSound()
{
    if (m_sounds_enabled)
    {
#ifdef NEW_SOUNDS //AVO: use custom sounds
        if (bMisfire)
        {
            //TODO: make sure correct sound is loaded in CWeaponMagazined::Load(LPCSTR section)
            if (m_sounds.FindSoundItem("sndReloadMisfire", false))
                PlaySound("sndReloadMisfire", get_LastFP());
            else
                PlaySound("sndReload", get_LastFP());
        }
        else
        {
            if (iAmmoElapsed == 0)
                if (m_sounds.FindSoundItem("sndReloadEmpty", false))
                    PlaySound("sndReloadEmpty", get_LastFP());
                else
                    PlaySound("sndReload", get_LastFP());
            else
                PlaySound("sndReload", get_LastFP());
        }
#else
        PlaySound("sndReload", get_LastFP());
#endif //-AVO
    }
}

void CWeaponMagazined::switch2_Reload()
{
    CWeapon::FireEnd();

    PlayReloadSound();
    PlayAnimReload();
    SetPending(true);
}
void CWeaponMagazined::switch2_Hiding()
{
    OnZoomOut();
    CWeapon::FireEnd();

    if (m_sounds_enabled)
        PlaySound("sndHide", get_LastFP());

    PlayAnimHide();
    SetPending(true);
}

void CWeaponMagazined::switch2_Hidden()
{
    CWeapon::FireEnd();

    StopCurrentAnimWithoutCallback();

    signal_HideComplete();
    RemoveShotEffector();
}
void CWeaponMagazined::switch2_Showing()
{
    if (m_sounds_enabled)
        PlaySound("sndShow", get_LastFP());

    SetPending(true);
    PlayAnimShow();
}

bool CWeaponMagazined::Action(u16 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;

    //если оружие чем-то занято, то ничего не делать
    if (IsPending())
        return false;

    switch (cmd)
    {
    case kWPN_RELOAD:
    {
        if (flags & CMD_START)
            if (iAmmoElapsed < iMagazineSize || IsMisfire())
                Reload();
    }
        return true;
    case kWPN_FIREMODE_PREV:
    {
        if (flags & CMD_START)
        {
            OnPrevFireMode();
            return true;
        };
    }
    break;
    case kWPN_FIREMODE_NEXT:
    {
        if (flags & CMD_START)
        {
            OnNextFireMode();
            return true;
        };
    }
    break;
    }
    return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
    CScope* pScope = smart_cast<CScope*>(pIItem);
    CSilencer* pSilencer = smart_cast<CSilencer*>(pIItem);
    CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

    if (pScope && m_eScopeStatus == ALife::eAddonAttachable &&
        (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0 /*&&
                (m_scopes[cur_scope]->m_sScopeName == pIItem->object().cNameSect())*/)
    {
        auto it = m_scopes.begin();
        for (; it != m_scopes.end(); it++)
        {
            if (pSettings->r_string((*it), "scope_name") == pIItem->object().cNameSect())
                return true;
        }
        return false;
    }
    else if (pSilencer && m_eSilencerStatus == ALife::eAddonAttachable &&
        (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
        (m_sSilencerName == pIItem->object().cNameSect()))
        return true;
    else if (pGrenadeLauncher && m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
        (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
        (m_sGrenadeLauncherName == pIItem->object().cNameSect()))
        return true;
    else
        return inherited::CanAttach(pIItem);
}

bool CWeaponMagazined::CanDetach(const char* item_section_name)
{
    if (m_eScopeStatus == ALife::eAddonAttachable &&
        0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope)) /* &&
           (m_scopes[cur_scope]->m_sScopeName	== item_section_name))*/
    {
        auto it = m_scopes.begin();
        for (; it != m_scopes.end(); it++)
        {
            if (pSettings->r_string((*it), "scope_name") == item_section_name)
                return true;
        }
        return false;
    }
    //	   return true;
    else if (m_eSilencerStatus == ALife::eAddonAttachable &&
        0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) && (m_sSilencerName == item_section_name))
        return true;
    else if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
        0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
        (m_sGrenadeLauncherName == item_section_name))
        return true;
    else
        return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazined::Attach(PIItem pIItem, bool b_send_event)
{
    bool result = false;

    CScope* pScope = smart_cast<CScope*>(pIItem);
    CSilencer* pSilencer = smart_cast<CSilencer*>(pIItem);
    CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

    if (pScope && m_eScopeStatus == ALife::eAddonAttachable &&
        (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0 /*&&
       (m_scopes[cur_scope]->m_sScopeName == pIItem->object().cNameSect())*/)
    {
        auto it = m_scopes.begin();
        for (; it != m_scopes.end(); it++)
        {
            if (pSettings->r_string((*it), "scope_name") == pIItem->object().cNameSect())
                m_cur_scope = u8(it - m_scopes.begin());
        }
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
        result = true;
    }
    else if (pSilencer && m_eSilencerStatus == ALife::eAddonAttachable &&
        (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
        (m_sSilencerName == pIItem->object().cNameSect()))
    {
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
        result = true;
    }
    else if (pGrenadeLauncher && m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
        (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
        (m_sGrenadeLauncherName == pIItem->object().cNameSect()))
    {
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
        result = true;
    }

    if (result)
    {
        if (b_send_event && OnServer())
        {
            //уничтожить подсоединенную вещь из инвентаря
            //.			pIItem->Drop					();
            pIItem->object().DestroyObject();
        };

        UpdateAddonsVisibility();
        InitAddons();

        return true;
    }
    else
        return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazined::DetachScope(const char* item_section_name, bool b_spawn_item)
{
    bool detached = false;
    auto it = m_scopes.begin();
    for (; it != m_scopes.end(); it++)
    {
        LPCSTR iter_scope_name = pSettings->r_string((*it), "scope_name");
        if (!xr_strcmp(iter_scope_name, item_section_name))
        {
            m_cur_scope = 0;
            detached = true;
        }
    }
    return detached;
}

bool CWeaponMagazined::Detach(const char* item_section_name, bool b_spawn_item)
{
    if (m_eScopeStatus == ALife::eAddonAttachable && DetachScope(item_section_name, b_spawn_item))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
        {
            Msg("ERROR: scope addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonScope;

        UpdateAddonsVisibility();
        InitAddons();

        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }
    else if (m_eSilencerStatus == ALife::eAddonAttachable && (m_sSilencerName == item_section_name))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
        {
            Msg("ERROR: silencer addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;

        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }
    else if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable && (m_sGrenadeLauncherName == item_section_name))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0)
        {
            Msg("ERROR: grenade launcher addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

        UpdateAddonsVisibility();
        InitAddons();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }
    else
        return inherited::Detach(item_section_name, b_spawn_item);
    ;
}
/*
void CWeaponMagazined::LoadAddons()
{
    m_zoom_params.m_fIronSightZoomFactor = READ_IF_EXISTS( pSettings, r_float, cNameSect(), "ironsight_zoom_factor",
50.0f );

}
*/
void CWeaponMagazined::InitAddons()
{
    m_zoom_params.m_fIronSightZoomFactor =
        READ_IF_EXISTS(pSettings, r_float, cNameSect(), "ironsight_zoom_factor", 50.0f);
    if (IsScopeAttached())
    {
        shared_str scope_tex_name;
        if (m_eScopeStatus == ALife::eAddonAttachable)
        {
            // m_scopes[cur_scope]->m_sScopeName = pSettings->r_string(cNameSect(), "scope_name");
            // m_scopes[cur_scope]->m_iScopeX	 = pSettings->r_s32(cNameSect(),"scope_x");
            // m_scopes[cur_scope]->m_iScopeY	 = pSettings->r_s32(cNameSect(),"scope_y");

            scope_tex_name = pSettings->r_string(GetScopeName(), "scope_texture");
            m_zoom_params.m_fScopeZoomFactor = pSettings->r_float(GetScopeName(), "scope_zoom_factor");
            m_zoom_params.m_sUseZoomPostprocess =
                READ_IF_EXISTS(pSettings, r_string, GetScopeName(), "scope_nightvision", 0);
            m_zoom_params.m_bUseDynamicZoom =
                READ_IF_EXISTS(pSettings, r_bool, GetScopeName(), "scope_dynamic_zoom", false);
            m_zoom_params.m_sUseBinocularVision =
                READ_IF_EXISTS(pSettings, r_string, GetScopeName(), "scope_alive_detector", 0);
            m_fRTZoomFactor = m_zoom_params.m_fScopeZoomFactor;
            if (m_UIScope)
            {
                xr_delete(m_UIScope);
            }

            if (!GEnv.isDedicatedServer)
            {
                m_UIScope = new CUIWindow();
                createWpnScopeXML();
                CUIXmlInit::InitWindow(*pWpnScopeXml, scope_tex_name.c_str(), 0, m_UIScope);
            }
        }
    }
    else
    {
        if (m_UIScope)
        {
            xr_delete(m_UIScope);
        }

        if (IsZoomEnabled())
        {
            m_zoom_params.m_fIronSightZoomFactor = pSettings->r_float(cNameSect(), "scope_zoom_factor");
        }
    }

    if (IsSilencerAttached() /* && SilencerAttachable() */)
    {
        m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
        m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
        m_sSndShotCurrent = "sndSilencerShot";

        //подсветка от выстрела
        LoadLights(*cNameSect(), "silencer_");
        ApplySilencerKoeffs();
    }
    else
    {
        m_sFlameParticlesCurrent = m_sFlameParticles;
        m_sSmokeParticlesCurrent = m_sSmokeParticles;
        m_sSndShotCurrent = "sndShot";

        //подсветка от выстрела
        LoadLights(*cNameSect(), "");
        ResetSilencerKoeffs();
    }

    inherited::InitAddons();
}

void CWeaponMagazined::LoadSilencerKoeffs()
{
    if (m_eSilencerStatus == ALife::eAddonAttachable)
    {
        LPCSTR sect = m_sSilencerName.c_str();
        m_silencer_koef.hit_power = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_hit_power_k", 1.0f);
        m_silencer_koef.hit_impulse = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_hit_impulse_k", 1.0f);
        m_silencer_koef.bullet_speed = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_speed_k", 1.0f);
        m_silencer_koef.fire_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "fire_dispersion_base_k", 1.0f);
        m_silencer_koef.cam_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_k", 1.0f);
        m_silencer_koef.cam_disper_inc = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_inc_k", 1.0f);
    }

    clamp(m_silencer_koef.hit_power, 0.0f, 1.0f);
    clamp(m_silencer_koef.hit_impulse, 0.0f, 1.0f);
    clamp(m_silencer_koef.bullet_speed, 0.0f, 1.0f);
    clamp(m_silencer_koef.fire_dispersion, 0.0f, 3.0f);
    clamp(m_silencer_koef.cam_dispersion, 0.0f, 1.0f);
    clamp(m_silencer_koef.cam_disper_inc, 0.0f, 1.0f);
}

void CWeaponMagazined::ApplySilencerKoeffs() { cur_silencer_koef = m_silencer_koef; }
void CWeaponMagazined::ResetSilencerKoeffs() { cur_silencer_koef.Reset(); }
void CWeaponMagazined::PlayAnimShow()
{
    VERIFY(GetState() == eShowing);
    PlayHUDMotion("anm_show", false, this, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
    VERIFY(GetState() == eHiding);
    PlayHUDMotion("anm_hide", true, this, GetState());
}

void CWeaponMagazined::PlayAnimReload()
{
    auto state = GetState();
    VERIFY(state == eReload);
#ifdef NEW_ANIMS //AVO: use new animations
    if (bMisfire)
        if (isHUDAnimationExist("anm_reload_misfire"))
            PlayHUDMotion("anm_reload_misfire", true, this, state);
        else
            PlayHUDMotion("anm_reload", true, this, state);
    else
    {
        if (iAmmoElapsed == 0)
            if (isHUDAnimationExist("anm_reload_empty"))
                PlayHUDMotion("anm_reload_empty", true, this, state);
            else
                PlayHUDMotion("anm_reload", true, this, state);
        else
            PlayHUDMotion("anm_reload", true, this, state);
    }
#else
    PlayHUDMotion("anm_reload", true, this, state);
#endif //-NEW_ANIM
}

void CWeaponMagazined::PlayAnimAim() { PlayHUDMotion("anm_idle_aim", true, nullptr, GetState()); }
void CWeaponMagazined::PlayAnimIdle()
{
    if (GetState() != eIdle)
        return;
    if (IsZoomed())
    {
        PlayAnimAim();
    }
    else
        inherited::PlayAnimIdle();
}

void CWeaponMagazined::PlayAnimShoot()
{
    VERIFY(GetState() == eFire);
    PlayHUDMotion("anm_shots", false, this, GetState());
}

void CWeaponMagazined::OnZoomIn()
{
    inherited::OnZoomIn();

    if (GetState() == eIdle)
        PlayAnimIdle();

    //Alundaio: callback not sure why vs2013 gives error, it's fine
#ifdef EXTENDED_WEAPON_CALLBACKS
    CGameObject *object = smart_cast<CGameObject*>(H_Parent());
    if (object)
        object->callback(GameObject::eOnWeaponZoomIn)(object->lua_game_object(),this->lua_game_object());
#endif
    //-Alundaio

    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (pActor)
    {
        CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>(pActor->Cameras().GetCamEffector(eCEZoom));
        if (!S)
        {
            S = (CEffectorZoomInertion*)pActor->Cameras().AddCamEffector(new CEffectorZoomInertion());
            S->Init(this);
        };
        S->SetRndSeed(pActor->GetZoomRndSeed());
        R_ASSERT(S);
    }
}
void CWeaponMagazined::OnZoomOut()
{
    if (!IsZoomed())
        return;

    inherited::OnZoomOut();

    if (GetState() == eIdle)
        PlayAnimIdle();

    //Alundaio
#ifdef EXTENDED_WEAPON_CALLBACKS
    CGameObject *object = smart_cast<CGameObject*>(H_Parent());
    if (object)
        object->callback(GameObject::eOnWeaponZoomOut)(object->lua_game_object(), this->lua_game_object());
#endif
    //-Alundaio

    CActor* pActor = smart_cast<CActor*>(H_Parent());

    if (pActor)
        pActor->Cameras().RemoveCamEffector(eCEZoom);
}

//переключение режимов стрельбы одиночными и очередями
bool CWeaponMagazined::SwitchMode()
{
    if (eIdle != GetState() || IsPending())
        return false;

    if (SingleShotMode())
        m_iQueueSize = WEAPON_ININITE_QUEUE;
    else
        m_iQueueSize = 1;

    PlaySound("sndEmptyClick", get_LastFP());

    return true;
}

void CWeaponMagazined::OnNextFireMode()
{
    if (!m_bHasDifferentFireModes)
        return;
    if (GetState() != eIdle)
        return;
    m_iCurFireMode = (m_iCurFireMode + 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
};

void CWeaponMagazined::OnPrevFireMode()
{
    if (!m_bHasDifferentFireModes)
        return;
    if (GetState() != eIdle)
        return;
    m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
};

void CWeaponMagazined::OnH_A_Chield()
{
    if (m_bHasDifferentFireModes)
    {
        CActor* actor = smart_cast<CActor*>(H_Parent());
        if (!actor)
            SetQueueSize(-1);
        else
            SetQueueSize(GetCurrentFireMode());
    };
    inherited::OnH_A_Chield();
};

void CWeaponMagazined::SetQueueSize(int size) { m_iQueueSize = size; };
float CWeaponMagazined::GetWeaponDeterioration()
{
    // modified by Peacemaker [17.10.08]
    //	if (!m_bHasDifferentFireModes || m_iPrefferedFireMode == -1 || u32(GetCurrentFireMode()) <=
    // u32(m_iPrefferedFireMode))
    //		return inherited::GetWeaponDeterioration();
    //	return m_iShotNum*conditionDecreasePerShot;
    return (m_iShotNum == 1) ? conditionDecreasePerShot : conditionDecreasePerQueueShot;
};

void CWeaponMagazined::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(m_iQueueSize, output_packet);
    save_data(m_iShotNum, output_packet);
    save_data(m_iCurFireMode, output_packet);
}

void CWeaponMagazined::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_iQueueSize, input_packet);
    SetQueueSize(m_iQueueSize);
    load_data(m_iShotNum, input_packet);
    load_data(m_iCurFireMode, input_packet);
}

void CWeaponMagazined::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);

    P.w_u8(u8(m_iCurFireMode & 0x00ff));
}

void CWeaponMagazined::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);

    m_iCurFireMode = P.r_u8();
    SetQueueSize(GetCurrentFireMode());
}

#include "string_table.h"
bool CWeaponMagazined::GetBriefInfo(II_BriefInfo& info)
{
    VERIFY(m_pInventory);
    string32 int_str;

    int ae = GetAmmoElapsed();
    xr_sprintf(int_str, "%d", ae);
    info.cur_ammo = int_str;

    if (HasFireModes())
    {
        if (m_iQueueSize == WEAPON_ININITE_QUEUE)
        {
            info.fire_mode = "A";
        }
        else
        {
            xr_sprintf(int_str, "%d", m_iQueueSize);
            info.fire_mode = int_str;
        }
    }
    else
        info.fire_mode = "";

    if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
    {
        return false;
    }
    GetSuitableAmmoTotal(); // update m_BriefInfo_CalcFrame
    info.grenade = "";

    const u32 at_size = m_ammoTypes.size();
    if (unlimited_ammo() || at_size == 0)
    {
        info.fmj_ammo._set("--");
        info.ap_ammo._set("--");
        info.third_ammo._set("--"); //Alundaio
    }
    else
    {
        // GetSuitableAmmoTotal(); //mp = all type
        //Alundaio: Added third ammo type and cleanup
        info.fmj_ammo._set("");
        info.ap_ammo._set("");
        info.third_ammo._set("");

        if (at_size >= 1)
        {
            xr_sprintf(int_str, "%d", GetAmmoCount(0));
            info.fmj_ammo._set(int_str);
        }
        if (at_size >= 2)
        {
            xr_sprintf(int_str, "%d", GetAmmoCount(1));
            info.ap_ammo._set(int_str);
        }
        if (at_size >= 3)
        {
            xr_sprintf(int_str, "%d", GetAmmoCount(2));
            info.third_ammo._set(int_str);
        }
        //-Alundaio
    }

    if (ae != 0 && m_magazine.size() != 0)
    {
        LPCSTR ammo_type = m_ammoTypes[m_magazine.back().m_LocalAmmoType].c_str();
        info.name = StringTable().translate(pSettings->r_string(ammo_type, "inv_name_short"));
        info.icon = ammo_type;
    }
    else
    {
        LPCSTR ammo_type = m_ammoTypes[m_ammoType].c_str();
        info.name = StringTable().translate(pSettings->r_string(ammo_type, "inv_name_short"));
        info.icon = ammo_type;
    }
    return true;
}

bool CWeaponMagazined::install_upgrade_impl(LPCSTR section, bool test)
{
    bool result = inherited::install_upgrade_impl(section, test);

    LPCSTR str;
    // fire_modes = 1, 2, -1
    bool result2 = process_if_exists_set(section, "fire_modes", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        int ModesCount = _GetItemCount(str);
        m_aFireModes.clear();
        for (int i = 0; i < ModesCount; ++i)
        {
            string16 sItem;
            _GetItem(str, i, sItem);
            m_aFireModes.push_back((s8)atoi(sItem));
        }
        m_iCurFireMode = ModesCount - 1;
    }
    result |= result2;

    result |= process_if_exists_set(
        section, "base_dispersioned_bullets_count", &CInifile::r_s32, m_iBaseDispersionedBulletsCount, test);
    result |= process_if_exists_set(
        section, "base_dispersioned_bullets_speed", &CInifile::r_float, m_fBaseDispersionedBulletsSpeed, test);

    // sounds (name of the sound, volume (0.0 - 1.0), delay (sec))
    result2 = process_if_exists_set(section, "snd_draw", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_draw", "sndShow", false, m_eSoundShow);
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_holster", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_holster", "sndHide", false, m_eSoundHide);
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_shoot", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
#ifdef LAYERED_SND_SHOOT
        m_layered_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
#else
        m_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
#endif
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_empty", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", false, m_eSoundEmptyClick);
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_reload", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_reload", "sndReload", true, m_eSoundReload);
    }
    result |= result2;

#ifdef NEW_SOUNDS //AVO: custom sounds go here
    result2 = process_if_exists_set(section, "snd_reload_empty", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", true, m_eSoundReloadEmpty);
    }
    result |= result2;
#endif //-NEW_SOUNDS

    // snd_shoot1     = weapons\ak74u_shot_1 ??
    // snd_shoot2     = weapons\ak74u_shot_2 ??
    // snd_shoot3     = weapons\ak74u_shot_3 ??

    if (m_eSilencerStatus == ALife::eAddonAttachable || m_eSilencerStatus == ALife::eAddonPermanent)
    {
        result |= process_if_exists_set(
            section, "silencer_flame_particles", &CInifile::r_string, m_sSilencerFlameParticles, test);
        result |= process_if_exists_set(
            section, "silencer_smoke_particles", &CInifile::r_string, m_sSilencerSmokeParticles, test);

        result2 = process_if_exists_set(section, "snd_silncer_shot", &CInifile::r_string, str, test);
        if (result2 && !test)
        {
#ifdef LAYERED_SND_SHOOT
            m_layered_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
#else
            m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
#endif
        }
        result |= result2;
    }

    // fov for zoom mode
    result |= process_if_exists(
        section, "ironsight_zoom_factor", &CInifile::r_float, m_zoom_params.m_fIronSightZoomFactor, test);

    if (IsScopeAttached())
    {
        // if ( m_eScopeStatus == ALife::eAddonAttachable )
        {
            result |= process_if_exists(
                section, "scope_zoom_factor", &CInifile::r_float, m_zoom_params.m_fScopeZoomFactor, test);
        }
    }
    else
    {
        if (IsZoomEnabled())
        {
            result |= process_if_exists(
                section, "scope_zoom_factor", &CInifile::r_float, m_zoom_params.m_fIronSightZoomFactor, test);
        }
    }

    return result;
}
//текущая дисперсия (в радианах) оружия с учетом используемого патрона и недисперсионных пуль
float CWeaponMagazined::GetFireDispersion(float cartridge_k, bool for_crosshair)
{
    float fire_disp = GetBaseDispersion(cartridge_k);
    if (for_crosshair || !m_iBaseDispersionedBulletsCount || !m_iShotNum ||
        m_iShotNum > m_iBaseDispersionedBulletsCount)
    {
        fire_disp = inherited::GetFireDispersion(cartridge_k);
    }
    return fire_disp;
}
void CWeaponMagazined::FireBullet(const Fvector& pos, const Fvector& shot_dir, float fire_disp,
    const CCartridge& cartridge, u16 parent_id, u16 weapon_id, bool send_hit)
{
    if (m_iBaseDispersionedBulletsCount)
    {
        if (m_iShotNum <= 1)
        {
            m_fOldBulletSpeed = GetBulletSpeed();
            SetBulletSpeed(m_fBaseDispersionedBulletsSpeed);
        }
        else if (m_iShotNum > m_iBaseDispersionedBulletsCount)
        {
            SetBulletSpeed(m_fOldBulletSpeed);
        }
    }
    inherited::FireBullet(pos, shot_dir, fire_disp, cartridge, parent_id, weapon_id, send_hit);
}
