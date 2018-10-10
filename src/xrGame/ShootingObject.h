//////////////////////////////////////////////////////////////////////
// ShootingObject.h: интерфейс для семейства стреляющих объектов
//					 (оружие и осколочные гранаты)
//					 обеспечивает набор хитов, звуков рикошетп
//////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "xrEngine/Render.h"
#include "anticheat_dumpable_object.h"

class CCartridge;
class CParticlesObject;
class IRender_Sector;

extern const Fvector zero_vel;

#define WEAPON_MATERIAL_NAME "objects" DELIMITER "bullet"

class CShootingObject : public IAnticheatDumpable
{
protected:
    CShootingObject();
    virtual ~CShootingObject();

    void reinit();
    void reload(LPCSTR section){};
    void Load(LPCSTR section);

    Fvector m_vCurrentShootDir;
    Fvector m_vCurrentShootPos;

private:
    float m_air_resistance_factor;

protected:
    // ID персонажа который иницировал действие
    u16 m_iCurrentParentID;

    //////////////////////////////////////////////////////////////////////////
    // Fire Params
    //////////////////////////////////////////////////////////////////////////
protected:
    virtual void LoadFireParams(LPCSTR section); //сила выстрела
    virtual bool SendHitAllowed(IGameObject* pUser);
    virtual void FireBullet(const Fvector& pos, const Fvector& dir, float fire_disp, const CCartridge& cartridge,
        u16 parent_id, u16 weapon_id, bool send_hit);
    void SetBulletSpeed(float new_speed) { m_fStartBulletSpeed = new_speed; }
    float GetBulletSpeed() { return m_fStartBulletSpeed; }
    virtual void FireStart();
    virtual void FireEnd();

public:
    IC BOOL IsWorking() const { return bWorking; }
    virtual BOOL ParentMayHaveAimBullet() { return FALSE; }
    virtual BOOL ParentIsActor() { return FALSE; }
protected:
    // Weapon fires now
    bool bWorking;

    float fOneShotTime;
    float modeShotTime;
    bool cycleDown;
    Fvector4 fvHitPower;
    Fvector4 fvHitPowerCritical;
    float fHitImpulse;

    //скорость вылета пули из ствола
    float m_fStartBulletSpeed;
    //максимальное расстояние стрельбы
    float fireDistance;

    //рассеивание во время стрельбы
    float fireDispersionBase;

    //счетчик времени, затрачиваемого на выстрел
    float fShotTimeCounter;

    struct SilencerKoeffs // value *= koef;
    {
        float hit_power;
        float hit_impulse;
        float bullet_speed;
        float fire_dispersion;
        float cam_dispersion;
        float cam_disper_inc;

        SilencerKoeffs() { Reset(); }
        IC void Reset()
        {
            hit_power = 1.0f;
            hit_impulse = 1.0f;
            bullet_speed = 1.0f;
            fire_dispersion = 1.0f;
            cam_dispersion = 1.0f;
            cam_disper_inc = 1.0f;
        }
    }; // SilencerKoeffs
    SilencerKoeffs m_silencer_koef;

public:
    SilencerKoeffs cur_silencer_koef;

protected:
    //для сталкеров, чтоб они знали эффективные границы использования
    //оружия
    float m_fMinRadius;
    float m_fMaxRadius;

protected:
    Fcolor light_base_color;
    float light_base_range;
    Fcolor light_build_color;
    float light_build_range;
    ref_light light_render;
    float light_var_color;
    float light_var_range;
    float light_lifetime;
    u32 light_frame;
    float light_time;
    //включение подсветки во время выстрела
    bool m_bLightShotEnabled;

protected:
    void Light_Create();
    void Light_Destroy();

    void Light_Start();
    void Light_Render(const Fvector& P);

    virtual void LoadLights(LPCSTR section, LPCSTR prefix);
    virtual void RenderLight();
    virtual void UpdateLight();
    virtual void StopLight();
    virtual bool IsHudModeNow() = 0;
    //////////////////////////////////////////////////////////////////////////
    // партикловая система
    //////////////////////////////////////////////////////////////////////////
protected:
    //функции родительского объекта
    virtual const Fvector& get_CurrentFirePoint() = 0;
    virtual const Fmatrix& get_ParticlesXFORM() = 0;
    virtual void ForceUpdateFireParticles(){};

    ////////////////////////////////////////////////
    //общие функции для работы с партиклами оружия
    void StartParticles(CParticlesObject*& pParticles, LPCSTR particles_name, const Fvector& pos,
        const Fvector& vel = zero_vel, bool auto_remove_flag = false);
    void StopParticles(CParticlesObject*& pParticles);
    void UpdateParticles(CParticlesObject*& pParticles, const Fvector& pos, const Fvector& vel = zero_vel);

    void LoadShellParticles(LPCSTR section, LPCSTR prefix);
    void LoadFlameParticles(LPCSTR section, LPCSTR prefix);

    ////////////////////////////////////////////////
    //спецефические функции для партиклов
    //партиклы огня
    void StartFlameParticles();
    void StopFlameParticles();
    void UpdateFlameParticles();

    //партиклы дыма
    void StartSmokeParticles(const Fvector& play_pos, const Fvector& parent_vel);

    //партиклы полосы от пули
    void StartShotParticles();

    //партиклы гильз
    void OnShellDrop(const Fvector& play_pos, const Fvector& parent_vel);

protected:
    //имя пратиклов для гильз
    shared_str m_sShellParticles;

public:
    Fvector vLoadedShellPoint;
    float m_fPredBulletTime;
    float m_fTimeToAim;
    bool m_bUseAimBullet;

protected:
    //имя пратиклов для огня
    shared_str m_sFlameParticlesCurrent;
    //для выстрела 1м и 2м видом стрельбы
    shared_str m_sFlameParticles;
    //объект партиклов огня
    CParticlesObject* m_pFlameParticles;

    //имя пратиклов для дыма
    shared_str m_sSmokeParticlesCurrent;
    shared_str m_sSmokeParticles;

    //имя партиклов следа от пули
    shared_str m_sShotParticles;

public:
    virtual void DumpActiveParams(shared_str const& section_name, CInifile& dst_ini) const;
};
