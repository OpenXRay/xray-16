#pragma once

#include "Weapon.h"
#include "HudSound.h"
#include "ai_sounds.h"

class ENGINE_API CMotionDef;

//размер очереди считается бесконечность
//заканчиваем стрельбу, только, если кончились патроны
#define WEAPON_ININITE_QUEUE -1

class CWeaponMagazined : public CWeapon
{
private:
    typedef CWeapon inherited;

protected:
    //звук текущего выстрела
    shared_str m_sSndShotCurrent;

    //дополнительная информация о глушителе
    LPCSTR m_sSilencerFlameParticles;
    LPCSTR m_sSilencerSmokeParticles;

    ESoundTypes m_eSoundShow;
    ESoundTypes m_eSoundHide;
    ESoundTypes m_eSoundShot;
    ESoundTypes m_eSoundEmptyClick;
    ESoundTypes m_eSoundReload;
#ifdef NEW_SOUNDS //AVO: new sounds go here
    ESoundTypes m_eSoundReloadEmpty;
    ESoundTypes m_eSoundReloadMisfire;
#endif //-NEW_SOUNDS

    bool m_sounds_enabled;
    // General
    //кадр момента пересчета UpdateSounds
    u32 dwUpdateSounds_Frame;

protected:
    virtual void OnMagazineEmpty();

    virtual void switch2_Idle();
    virtual void switch2_Fire();
    virtual void switch2_Empty();
    virtual void switch2_Reload();
    virtual void switch2_Hiding();
    virtual void switch2_Hidden();
    virtual void switch2_Showing();

    virtual void OnShot();

    virtual void OnEmptyClick();

    virtual void OnAnimationEnd(u32 state);
    virtual void OnStateSwitch(u32 S, u32 oldState);

    virtual void UpdateSounds();

    bool TryReload();

protected:
    virtual void ReloadMagazine();
    void ApplySilencerKoeffs();
    void ResetSilencerKoeffs();

    virtual void state_Fire(float dt);
    virtual void state_MagEmpty(float dt);
    virtual void state_Misfire(float dt);

public:
    CWeaponMagazined(ESoundTypes eSoundType = SOUND_TYPE_WEAPON_SUBMACHINEGUN);
    virtual ~CWeaponMagazined();

    virtual void Load(LPCSTR section);
    void LoadSilencerKoeffs();
    virtual CWeaponMagazined* cast_weapon_magazined() { return this; }
    virtual void SetDefaults();
    virtual void FireStart();
    virtual void FireEnd();
    virtual void Reload();

    virtual void UpdateCL();
    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);

    virtual void OnH_A_Chield();

    virtual bool Attach(PIItem pIItem, bool b_send_event);
    virtual bool Detach(const char* item_section_name, bool b_spawn_item);
    bool DetachScope(const char* item_section_name, bool b_spawn_item);
    virtual bool CanAttach(PIItem pIItem);
    virtual bool CanDetach(const char* item_section_name);

    virtual void InitAddons();

    virtual bool Action(u16 cmd, u32 flags);
    bool IsAmmoAvailable();
    virtual void UnloadMagazine(bool spawn_ammo = true);

    virtual bool GetBriefInfo(II_BriefInfo& info);

public:
    virtual bool SwitchMode();
    virtual bool SingleShotMode() { return 1 == m_iQueueSize; }
    virtual void SetQueueSize(int size);
    IC int GetQueueSize() const { return m_iQueueSize; };
    virtual bool StopedAfterQueueFired() { return m_bStopedAfterQueueFired; }
    virtual void StopedAfterQueueFired(bool value) { m_bStopedAfterQueueFired = value; }
    virtual float GetFireDispersion(float cartridge_k, bool for_crosshair = false);

protected:
    //максимальный размер очереди, которой можно стрельнуть
    int m_iQueueSize;
    //количество реально выстреляных патронов
    int m_iShotNum;
    //после какого патрона, при непрерывной стрельбе, начинается отдача (сделано из-за Абакана)
    int m_iBaseDispersionedBulletsCount;
    //скорость вылета патронов, на которые не влияет отдача (сделано из-за Абакана)
    float m_fBaseDispersionedBulletsSpeed;
    //скорость вылета остальных патронов
    float m_fOldBulletSpeed;
    Fvector m_vStartPos, m_vStartDir;
    //флаг того, что мы остановились после того как выстреляли
    //ровно столько патронов, сколько было задано в m_iQueueSize
    bool m_bStopedAfterQueueFired;
    //флаг того, что хотя бы один выстрел мы должны сделать
    //(даже если очень быстро нажали на курок и вызвалось FireEnd)
    bool m_bFireSingleShot;
    //режимы стрельбы
    bool m_bHasDifferentFireModes;
    xr_vector<s8> m_aFireModes;
    int m_iCurFireMode;
    int m_iPrefferedFireMode;

    //переменная блокирует использование
    //только разных типов патронов
    bool m_bLockType;

public:
    virtual void OnZoomIn();
    virtual void OnZoomOut();
    void OnNextFireMode();
    void OnPrevFireMode();
    bool HasFireModes() { return m_bHasDifferentFireModes; }

    int GetCurrentFireMode() override
    {
        //AVO: fixed crash due to original GSC assumption that CWeaponMagazined will always have firemodes specified in configs.
        if (HasFireModes())
            return m_aFireModes[m_iCurFireMode];
        return 1;
    }

    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

protected:
    virtual bool install_upgrade_impl(LPCSTR section, bool test);

    virtual bool AllowFireWhileWorking() { return false; }
    //виртуальные функции для проигрывания анимации HUD
    virtual void PlayAnimShow();
    virtual void PlayAnimHide();
    virtual void PlayAnimReload();
    virtual void PlayAnimIdle();
    virtual void PlayAnimShoot();
    virtual void PlayReloadSound();
    virtual void PlayAnimAim();

    virtual int ShotsFired() { return m_iShotNum; }
    virtual float GetWeaponDeterioration();

    virtual void FireBullet(const Fvector& pos, const Fvector& dir, float fire_disp, const CCartridge& cartridge,
        u16 parent_id, u16 weapon_id, bool send_hit);

    //AVO: for custom added sounds check if sound exists
    bool WeaponSoundExist(pcstr section, pcstr sound_name) const;

    //Alundaio: LAYERED_SND_SHOOT
#ifdef LAYERED_SND_SHOOT
    HUD_SOUND_COLLECTION_LAYERED m_layered_sounds;
#endif
    //-Alundaio
};
