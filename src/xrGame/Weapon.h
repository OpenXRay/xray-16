#pragma once

#include "xrPhysics/PhysicsShell.h"
#include "WeaponAmmo.h"
#include "PHShellCreator.h"

#include "ShootingObject.h"
#include "hud_item_object.h"
#include "Actor_Flags.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "firedeps.h"
#include "game_cl_single.h"
#include "first_bullet_controller.h"

#include "CameraRecoil.h"

class CEntity;
class ENGINE_API CMotionDef;
class CSE_ALifeItemWeapon;
class CSE_ALifeItemWeaponAmmo;
class CWeaponMagazined;
class CParticlesObject;
class CUIWindow;
class CBinocularsVision;
class CNightVisionEffector;

class CWeapon : public CHudItemObject, public CShootingObject
{
    typedef CHudItemObject inherited;

public:
    CWeapon();
    virtual ~CWeapon();

    // Generic
    virtual void Load(LPCSTR section);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);

    virtual CWeapon* cast_weapon() { return this; }
    virtual CWeaponMagazined* cast_weapon_magazined() { return 0; }
    // serialization
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);
    virtual BOOL net_SaveRelevant() { return inherited::net_SaveRelevant(); }
    virtual void UpdateCL();
    virtual void shedule_Update(u32 dt);

    virtual void renderable_Render();
    virtual void render_hud_mode();
    virtual bool need_renderable();

    virtual void render_item_ui();
    virtual bool render_item_ui_query();

    virtual void OnH_B_Chield();
    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void OnH_A_Independent();
    virtual void OnEvent(NET_Packet& P, u16 type); // {inherited::OnEvent(P,type);}

    virtual void Hit(SHit* pHDS);

    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual void create_physic_shell();
    virtual void activate_physic_shell();
    virtual void setup_physic_shell();

    virtual void SwitchState(u32 S);

    virtual void OnActiveItem();
    virtual void OnHiddenItem();
    virtual void SendHiddenItem(); // same as OnHiddenItem but for client... (sends message to a server)...

public:
    virtual bool can_kill() const;
    virtual CInventoryItem* can_kill(CInventory* inventory) const;
    virtual const CInventoryItem* can_kill(const xr_vector<const CGameObject*>& items) const;
    virtual bool ready_to_kill() const;
    virtual bool NeedToDestroyObject() const;
    virtual ALife::_TIME_ID TimePassedAfterIndependant() const;

protected:
    //время удаления оружия
    ALife::_TIME_ID m_dwWeaponRemoveTime;
    ALife::_TIME_ID m_dwWeaponIndependencyTime;

    virtual bool IsHudModeNow();

public:
    void signal_HideComplete();
    virtual bool Action(u16 cmd, u32 flags);

    enum EWeaponStates
    {
        eFire = eLastBaseState + 1,
        eFire2,
        eReload,
        eMisfire,
        eMagEmpty,
        eSwitch,
    };
    enum EWeaponSubStates
    {
        eSubstateReloadBegin = 0,
        eSubstateReloadInProcess,
        eSubstateReloadEnd,
    };
    enum
    {
        undefined_ammo_type = u8(-1)
    };

    IC BOOL IsValid() const { return iAmmoElapsed; }
    // Does weapon need's update?
    BOOL IsUpdating();

    BOOL IsMisfire() const;
    BOOL CheckForMisfire();

    BOOL AutoSpawnAmmo() const { return m_bAutoSpawnAmmo; };
    bool IsTriStateReload() const { return m_bTriStateReload; }
    EWeaponSubStates GetReloadState() const { return (EWeaponSubStates)m_sub_state; }
protected:
    bool m_bTriStateReload;

    // a misfire happens, you'll need to rearm weapon
    bool bMisfire;

    BOOL m_bAutoSpawnAmmo;
    virtual bool AllowBore();

public:
    u8 m_sub_state; // Alundaio: made public

    bool IsGrenadeLauncherAttached() const;
    bool IsScopeAttached() const;
    bool IsSilencerAttached() const;

    virtual bool GrenadeLauncherAttachable();
    virtual bool ScopeAttachable();
    virtual bool SilencerAttachable();

    ALife::EWeaponAddonStatus get_GrenadeLauncherStatus() const { return m_eGrenadeLauncherStatus; }
    ALife::EWeaponAddonStatus get_ScopeStatus() const { return m_eScopeStatus; }
    ALife::EWeaponAddonStatus get_SilencerStatus() const { return m_eSilencerStatus; }
    virtual bool UseScopeTexture() { return true; };
    //обновление видимости для косточек аддонов
    void UpdateAddonsVisibility();
    void UpdateHUDAddonsVisibility();
    //инициализация свойств присоединенных аддонов
    virtual void InitAddons();

    //для отоброажения иконок апгрейдов в интерфейсе
    int GetScopeX() { return pSettings->r_s32(m_scopes[m_cur_scope], "scope_x"); }
    int GetScopeY() { return pSettings->r_s32(m_scopes[m_cur_scope], "scope_y"); }
    int GetSilencerX() { return m_iSilencerX; }
    int GetSilencerY() { return m_iSilencerY; }
    int GetGrenadeLauncherX() { return m_iGrenadeLauncherX; }
    int GetGrenadeLauncherY() { return m_iGrenadeLauncherY; }
    const shared_str& GetGrenadeLauncherName() const { return m_sGrenadeLauncherName; }
    const shared_str GetScopeName() const { return pSettings->r_string(m_scopes[m_cur_scope], "scope_name"); }
    const shared_str& GetSilencerName() const { return m_sSilencerName; }
    IC void ForceUpdateAmmo() { m_BriefInfo_CalcFrame = 0; }
    u8 GetAddonsState() const { return m_flagsAddOnState; };
    void SetAddonsState(u8 st) { m_flagsAddOnState = st; } // dont use!!! for buy menu only!!!
protected:
    //состояние подключенных аддонов
    u8 m_flagsAddOnState;

    //возможность подключения различных аддонов
    ALife::EWeaponAddonStatus m_eScopeStatus;
    ALife::EWeaponAddonStatus m_eSilencerStatus;
    ALife::EWeaponAddonStatus m_eGrenadeLauncherStatus;

    //названия секций подключаемых аддонов
    shared_str m_sScopeName;
    shared_str m_sSilencerName;
    shared_str m_sGrenadeLauncherName;

    //смещение иконов апгрейдов в инвентаре
    int m_iScopeX, m_iScopeY;
    int m_iSilencerX, m_iSilencerY;
    int m_iGrenadeLauncherX, m_iGrenadeLauncherY;

protected:
    struct SZoomParams
    {
        bool m_bZoomEnabled; //разрешение режима приближения
        bool m_bHideCrosshairInZoom;
        bool m_bZoomDofEnabled;

        bool m_bIsZoomModeNow; //когда режим приближения включен
        float m_fCurrentZoomFactor; //текущий фактор приближения
        float m_fZoomRotateTime; //время приближения

        float m_fIronSightZoomFactor; //коэффициент увеличения прицеливания
        float m_fScopeZoomFactor; //коэффициент увеличения прицела

        float m_fZoomRotationFactor;

        Fvector m_ZoomDof;
        Fvector4 m_ReloadDof;
        Fvector4 m_ReloadEmptyDof;
        bool m_bUseDynamicZoom;
        shared_str m_sUseZoomPostprocess;
        shared_str m_sUseBinocularVision;
        CBinocularsVision* m_pVision;
        CNightVisionEffector* m_pNight_vision;

    } m_zoom_params;

    float m_fRTZoomFactor; // run-time zoom factor
    CUIWindow* m_UIScope;

public:
    IC bool IsZoomEnabled() const { return m_zoom_params.m_bZoomEnabled; }
    virtual void ZoomInc();
    virtual void ZoomDec();
    virtual void OnZoomIn();
    virtual void OnZoomOut();
    IC bool IsZoomed() const { return m_zoom_params.m_bIsZoomModeNow; };
    CUIWindow* ZoomTexture();

    bool ZoomHideCrosshair() { return m_zoom_params.m_bHideCrosshairInZoom || ZoomTexture(); }
    IC float GetZoomFactor() const { return m_zoom_params.m_fCurrentZoomFactor; }
    IC void SetZoomFactor(float f) { m_zoom_params.m_fCurrentZoomFactor = f; }
    virtual float CurrentZoomFactor();
    //показывает, что оружие находится в соостоянии поворота для приближенного прицеливания
    bool IsRotatingToZoom() const { return (m_zoom_params.m_fZoomRotationFactor < 1.f); }
    virtual u8 GetCurrentHudOffsetIdx();

    virtual float Weight() const;
    virtual u32 Cost() const;

public:
    virtual EHandDependence HandDependence() const { return eHandDependence; }
    bool IsSingleHanded() const { return m_bIsSingleHanded; }
public:
    IC LPCSTR strap_bone0() const { return m_strap_bone0; }
    IC LPCSTR strap_bone1() const { return m_strap_bone1; }
    IC void strapped_mode(bool value) { m_strapped_mode = value; }
    IC bool strapped_mode() const { return m_strapped_mode; }
protected:
    LPCSTR m_strap_bone0;
    LPCSTR m_strap_bone1;
    Fmatrix m_StrapOffset;
    bool m_strapped_mode;
    bool m_can_be_strapped;

    Fmatrix m_Offset;
    // 0-используется без участия рук, 1-одна рука, 2-две руки
    EHandDependence eHandDependence;
    bool m_bIsSingleHanded;

public:
    //загружаемые параметры
    Fvector vLoadedFirePoint;
    Fvector vLoadedFirePoint2;

private:
    firedeps m_current_firedeps;

protected:
    virtual void UpdateFireDependencies_internal();
    virtual void UpdatePosition(const Fmatrix& transform); //.
    virtual void UpdateXForm();
    virtual void UpdateHudAdditonal(Fmatrix&);
    IC void UpdateFireDependencies()
    {
        if (dwFP_Frame == Device.dwFrame)
            return;
        UpdateFireDependencies_internal();
    };

    virtual void LoadFireParams(LPCSTR section);

public:
    IC const Fvector& get_LastFP()
    {
        UpdateFireDependencies();
        return m_current_firedeps.vLastFP;
    }
    IC const Fvector& get_LastFP2()
    {
        UpdateFireDependencies();
        return m_current_firedeps.vLastFP2;
    }
    IC const Fvector& get_LastFD()
    {
        UpdateFireDependencies();
        return m_current_firedeps.vLastFD;
    }
    IC const Fvector& get_LastSP()
    {
        UpdateFireDependencies();
        return m_current_firedeps.vLastSP;
    }

    virtual const Fvector& get_CurrentFirePoint() { return get_LastFP(); }
    virtual const Fvector& get_CurrentFirePoint2() { return get_LastFP2(); }
    virtual const Fmatrix& get_ParticlesXFORM()
    {
        UpdateFireDependencies();
        return m_current_firedeps.m_FireParticlesXForm;
    }
    virtual void ForceUpdateFireParticles();
    virtual void debug_draw_firedeps();

protected:
    virtual void SetDefaults();

    virtual bool MovingAnimAllowedNow();
    virtual void OnStateSwitch(u32 S, u32 oldState);
    virtual void OnAnimationEnd(u32 state);

    //трассирование полета пули
    virtual void FireTrace(const Fvector& P, const Fvector& D);
    virtual float GetWeaponDeterioration();

    virtual void FireStart() { CShootingObject::FireStart(); }
    virtual void FireEnd();

    virtual void Reload();
    void StopShooting();

    // обработка визуализации выстрела
    virtual void OnShot(){};
    virtual void AddShotEffector();
    virtual void RemoveShotEffector();
    virtual void ClearShotEffector();
    virtual void StopShotEffector();

public:
    float GetBaseDispersion(float cartridge_k);
    float GetFireDispersion(bool with_cartridge, bool for_crosshair = false);
    virtual float GetFireDispersion(float cartridge_k, bool for_crosshair = false);
    virtual int ShotsFired() { return 0; }
    virtual int GetCurrentFireMode() { return 1; }
    //параметы оружия в зависимоти от его состояния исправности
    float GetConditionDispersionFactor() const;
    float GetConditionMisfireProbability() const;
    virtual float GetConditionToShow() const;

public:
    CameraRecoil cam_recoil; // simple mode (walk, run)
    CameraRecoil zoom_cam_recoil; // using zoom =(ironsight or scope)

protected:
    //фактор увеличения дисперсии при максимальной изношености
    //(на сколько процентов увеличится дисперсия)
    float fireDispersionConditionFactor;
    //вероятность осечки при максимальной изношености

    // modified by Peacemaker [17.10.08]
    //	float					misfireProbability;
    //	float					misfireConditionK;
    float misfireStartCondition; //изношенность, при которой появляется шанс осечки
    float misfireEndCondition; //изношеность при которой шанс осечки становится константным
    float misfireStartProbability; //шанс осечки при изношености больше чем misfireStartCondition
    float misfireEndProbability; //шанс осечки при изношености больше чем misfireEndCondition
    float conditionDecreasePerQueueShot; //увеличение изношености при выстреле очередью
    float conditionDecreasePerShot; //увеличение изношености при одиночном выстреле

public:
    float GetMisfireStartCondition() const { return misfireStartCondition; };
    float GetMisfireEndCondition() const { return misfireEndCondition; };
protected:
    struct SPDM
    {
        float m_fPDM_disp_base;
        float m_fPDM_disp_vel_factor;
        float m_fPDM_disp_accel_factor;
        float m_fPDM_disp_crouch;
        float m_fPDM_disp_crouch_no_acc;
    };
    SPDM m_pdm;

    float m_crosshair_inertion;
    first_bullet_controller m_first_bullet_controller;

protected:
    //для отдачи оружия
    Fvector m_vRecoilDeltaAngle;

    //для сталкеров, чтоб они знали эффективные границы использования
    //оружия
    float m_fMinRadius;
    float m_fMaxRadius;

protected:
    //для второго ствола
    void StartFlameParticles2();
    void StopFlameParticles2();
    void UpdateFlameParticles2();

protected:
    shared_str m_sFlameParticles2;
    //объект партиклов для стрельбы из 2-го ствола
    CParticlesObject* m_pFlameParticles2;

protected:
    int GetAmmoCount(u8 ammo_type) const;

public:
    IC int GetAmmoElapsed() const { return /*int(m_magazine.size())*/ iAmmoElapsed; }
    IC int GetAmmoMagSize() const { return iMagazineSize; }
    int GetSuitableAmmoTotal(bool use_item_to_spawn = false) const;

    void SetAmmoElapsed(int ammo_count);

    virtual void OnMagazineEmpty();
    void SpawnAmmo(u32 boxCurr = 0xffffffff, LPCSTR ammoSect = NULL, u32 ParentID = 0xffffffff);
    bool SwitchAmmoType(u32 flags);

    virtual float Get_PDM_Base() const { return m_pdm.m_fPDM_disp_base; };
    virtual float Get_PDM_Vel_F() const { return m_pdm.m_fPDM_disp_vel_factor; };
    virtual float Get_PDM_Accel_F() const { return m_pdm.m_fPDM_disp_accel_factor; };
    virtual float Get_PDM_Crouch() const { return m_pdm.m_fPDM_disp_crouch; };
    virtual float Get_PDM_Crouch_NA() const { return m_pdm.m_fPDM_disp_crouch_no_acc; };
    virtual float GetCrosshairInertion() const { return m_crosshair_inertion; };
    float GetFirstBulletDisp() const { return m_first_bullet_controller.get_fire_dispertion(); };
protected:
    int iAmmoElapsed; // ammo in magazine, currently
    int iMagazineSize; // size (in bullets) of magazine

    //для подсчета в GetSuitableAmmoTotal
    mutable int m_iAmmoCurrentTotal;
    mutable u32 m_BriefInfo_CalcFrame; //кадр на котором просчитали кол-во патронов
    bool m_bAmmoWasSpawned;

    virtual bool IsNecessaryItem(const shared_str& item_sect);

public:
    xr_vector<shared_str> m_ammoTypes;
    /*
        struct SScopes
        {
            shared_str			m_sScopeName;
            int					m_iScopeX;
            int					m_iScopeY;
        };
        DEFINE_VECTOR(SScopes*, SCOPES_VECTOR, SCOPES_VECTOR_IT);
        SCOPES_VECTOR			m_scopes;

        u8						cur_scope;
    */

    using SCOPES_VECTOR = xr_vector<shared_str>;
    SCOPES_VECTOR m_scopes;
    u8 m_cur_scope;

    CWeaponAmmo* m_pCurrentAmmo;
    u8 m_ammoType;
    //-	shared_str				m_ammoName; <== deleted
    bool m_bHasTracers;
    u8 m_u8TracerColorID;
    u8 m_set_next_ammoType_on_reload;
    // Multitype ammo support
    xr_vector<CCartridge> m_magazine;
    CCartridge m_DefaultCartridge;
    float m_fCurrentCartirdgeDisp;

    bool unlimited_ammo();
    IC bool can_be_strapped() const { return m_can_be_strapped; };

    float GetMagazineWeight(const decltype(m_magazine)& mag) const;

protected:
    u32 m_ef_main_weapon_type;
    u32 m_ef_weapon_type;

public:
    virtual u32 ef_main_weapon_type() const;
    virtual u32 ef_weapon_type() const;

    //Alundaio
    int GetAmmoCount_forType(shared_str const& ammo_type) const;
    virtual void set_ef_main_weapon_type(u32 type) { m_ef_main_weapon_type = type; };
    virtual void set_ef_weapon_type(u32 type) { m_ef_weapon_type = type; };
    virtual void SetAmmoType(u8 type) { m_ammoType = type; };
    u8 GetAmmoType() { return m_ammoType; }
    //-Alundaio

protected:
    // This is because when scope is attached we can't ask scope for these params
    // therefore we should hold them by ourself :-((
    float m_addon_holder_range_modifier;
    float m_addon_holder_fov_modifier;

public:
    virtual void modify_holder_params(float& range, float& fov) const;
    virtual bool use_crosshair() const { return true; }
    bool show_crosshair();
    bool show_indicators();
    virtual BOOL ParentMayHaveAimBullet();
    virtual BOOL ParentIsActor();

private:
    virtual bool install_upgrade_ammo_class(LPCSTR section, bool test);
    bool install_upgrade_disp(LPCSTR section, bool test);
    bool install_upgrade_hit(LPCSTR section, bool test);
    bool install_upgrade_addon(LPCSTR section, bool test);

protected:
    virtual bool install_upgrade_impl(LPCSTR section, bool test);

private:
    float m_hit_probability[egdCount];

public:
    const float& hit_probability() const;

private:
    Fvector m_overriden_activation_speed;
    bool m_activation_speed_is_overriden;
    virtual bool ActivationSpeedOverriden(Fvector& dest, bool clear_override);

    bool m_bRememberActorNVisnStatus;

public:
    virtual void SetActivationSpeedOverride(Fvector const& speed);
    bool GetRememberActorNVisnStatus() { return m_bRememberActorNVisnStatus; };
    virtual void EnableActorNVisnAfterZoom();

    virtual void DumpActiveParams(shared_str const& section_name, CInifile& dst_ini) const;
    virtual shared_str const GetAnticheatSectionName() const { return cNameSect(); };
};
