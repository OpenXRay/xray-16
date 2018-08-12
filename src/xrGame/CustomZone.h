#pragma once

#include "space_restrictor.h"
#include "xrEngine/Feel_Touch.h"

class CActor;
class CLAItem;
class CParticlesObject;
class CZoneEffector;

#define SMALL_OBJECT_RADIUS 0.6f

//информация о объекте, находящемся в зоне
struct SZoneObjectInfo
{
    SZoneObjectInfo()
        : object(NULL), zone_ignore(false), dw_time_in_zone(0), f_time_affected(Device.fTimeGlobal),
          small_object(false), nonalive_object(false)
    {
    }
    CGameObject* object;
    bool small_object;
    bool nonalive_object;
    //игнорирование объекта в зоне
    bool zone_ignore;
    //присоединенные партиклы
    xr_vector<CParticlesObject*> particles_vector;
    //время прибывания в зоне
    u32 dw_time_in_zone;
    float f_time_affected;

    bool operator==(const CGameObject* O) const { return object == O; }
};

class CCustomZone : public CSpaceRestrictor, public Feel::Touch
{
    typedef CSpaceRestrictor inherited;

public:
    CZoneEffector* m_actor_effector;

    CCustomZone();
    virtual ~CCustomZone();

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Import(NET_Packet& P);
    virtual void net_Export(NET_Packet& P);
    virtual void Load(LPCSTR section);
    virtual void net_Destroy();

    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    virtual void UpdateCL();
    virtual void UpdateWorkload(u32 dt);
    virtual void shedule_Update(u32 dt);
    virtual void enter_Zone(SZoneObjectInfo& io);
    virtual void exit_Zone(SZoneObjectInfo& io);
    virtual void feel_touch_new(IGameObject* O);
    virtual void feel_touch_delete(IGameObject* O);
    virtual bool feel_touch_contact(IGameObject* O);
    virtual bool feel_touch_on_contact(IGameObject* O);

    float effective_radius(float nearest_shape_radius);
    virtual void net_Relcase(IGameObject* O);
    virtual void OnEvent(NET_Packet& P, u16 type);

    float GetMaxPower() { return m_fMaxPower; }
    void SetMaxPower(float p) { m_fMaxPower = p; }
    //вычисление силы хита в зависимости от расстояния до центра зоны
    //относительный размер силы (от 0 до 1)
    float RelativePower(float dist, float nearest_shape_radius);
    //абсолютный размер
    float Power(float dist, float nearest_shape_radius);

    virtual CCustomZone* cast_custom_zone() { return this; }
    //различные состояния в которых может находиться зона
    typedef enum {
        eZoneStateIdle = 0, //состояние зоны, когда внутри нее нет активных объектов
        eZoneStateAwaking, //пробуждение зоны (объект попал в зону)
        eZoneStateBlowout, //выброс
        eZoneStateAccumulate, //накапливание энергии, после выброса
        eZoneStateDisabled,
        eZoneStateMax
    } EZoneState;

    IC ALife::EHitType GetHitType() { return m_eHitTypeBlowout; }
protected:
    enum EZoneFlags
    {
        eIgnoreNonAlive = (1 << 0),
        eIgnoreSmall = (1 << 1),
        eIgnoreArtefact = (1 << 2),
        eZoneIsActive = (1 << 3),
        eBlowoutWind = (1 << 4),
        eBlowoutLight = (1 << 5),
        eIdleLight = (1 << 6),
        eBlowoutWindActive = (1 << 7),
        eUseOnOffTime = (1 << 8),
        eIdleLightVolumetric = (1 << 9),
        eIdleLightShadow = (1 << 10),
        eAlwaysFastmode = (1 << 11),
        eFastMode = (1 << 12),
        eIdleObjectParticlesDontStop = (1 << 13),
        eAffectPickDOF = (1 << 14),
        eIdleLightR1 = (1 << 15),
        eBoltEntranceParticles = (1 << 16),
        eUseSecondaryHit = (1 << 17),
    };
    u32 m_owner_id;
    u32 m_ttl;
    Flags32 m_zone_flags;

    //максимальная сила заряда зоны
    float m_fMaxPower;
    //сила постоянного небольшого демеджа (для огненных и ядовитых мин)
    float m_fSecondaryHitPower;

    //линейный коэффициент затухания в зависимости от расстояния
    float m_fAttenuation;
    //процент удара зоны, который пойдет на физический импульс
    float m_fHitImpulseScale;
    //размер радиуса в процентах от оригинального,
    //где действует зона
    float m_fEffectiveRadius;

    //тип наносимого хита
    ALife::EHitType m_eHitTypeBlowout;
    EZoneState m_eZoneState;

    //текущее время пребывания зоны в определенном состоянии
    int m_iStateTime;
    int m_iPreviousStateTime;

    u32 m_TimeToDisable;
    u32 m_TimeToEnable;
    u32 m_TimeShift;
    u32 m_StartTime;

    //массив с временами, сколько каждое состояние должно
    //длиться (если 0, то мгновенно -1 - бесконечность,
    //-2 - вообще не должно вызываться)
    typedef svector<int, eZoneStateMax> StateTimeSVec;
    StateTimeSVec m_StateTime;

    virtual void SwitchZoneState(EZoneState new_state);
    virtual void OnStateSwitch(EZoneState new_state);
    virtual void CheckForAwaking();
    //обработка зоны в различных состояниях
    virtual bool IdleState();
    virtual bool AwakingState();
    virtual bool BlowoutState();
    virtual bool AccumulateState();

    virtual void UpdateSecondaryHit(){};

    virtual bool Enable();
    virtual bool Disable();
    void UpdateOnOffState();
    virtual void GoEnabledState();
    virtual void GoDisabledState();

public:
    bool IsEnabled() { return m_eZoneState != eZoneStateDisabled; };
    void ZoneEnable();
    void ZoneDisable();
    EZoneState ZoneState() { return m_eZoneState; }
protected:
    //воздействие зоной на объект
    virtual void Affect(SZoneObjectInfo* O) {}
    //воздействовать на все объекты в зоне
    void AffectObjects();

    u32 m_dwAffectFrameNum;

    //параметры для выброса, с какой задержкой
    //включать эффекты и логику
    u32 m_dwBlowoutParticlesTime;
    u32 m_dwBlowoutLightTime;
    u32 m_dwBlowoutSoundTime;
    u32 m_dwBlowoutExplosionTime;
    void UpdateBlowout();

    //ветер
    u32 m_dwBlowoutWindTimeStart;
    u32 m_dwBlowoutWindTimePeak;
    u32 m_dwBlowoutWindTimeEnd;
    //сила ветра (увеличение текущего) (0,1) когда в аномалию попадает актер
    float m_fBlowoutWindPowerMax;
    float m_fStoreWindPower;

    void StartWind();
    void StopWind();
    void UpdateWind();

    //время, через которое, зона перестает реагировать
    //на объект мертвый объект (-1 если не указано)
    int m_iDisableHitTime;
    //тоже самое но для маленьких объектов
    int m_iDisableHitTimeSmall;
    int m_iDisableIdleTime;

    ////////////////////////////////
    // имена партиклов зоны
    //обычное состояние зоны
    shared_str m_sIdleParticles;
    //выброс зоны
    shared_str m_sBlowoutParticles;
    BOOL m_bBlowoutOnce;
    shared_str m_sAccumParticles;
    shared_str m_sAwakingParticles;

    //появление большого и мальнекого объекта в зоне
    shared_str m_sEntranceParticlesSmall;
    shared_str m_sEntranceParticlesBig;
    //поражение большого и мальнекого объекта в зоне
    shared_str m_sHitParticlesSmall;
    shared_str m_sHitParticlesBig;
    //нахождение большого и мальнекого объекта в зоне
    shared_str m_sIdleObjectParticlesSmall;
    shared_str m_sIdleObjectParticlesBig;
    shared_str m_sBoltEntranceParticles;

    ref_sound m_idle_sound;
    ref_sound m_awaking_sound;
    ref_sound m_accum_sound;
    ref_sound m_blowout_sound;
    ref_sound m_hit_sound;
    ref_sound m_entrance_sound;

    //объект партиклов обычного состояния зоны
    CParticlesObject* m_pIdleParticles;

    //////////////////////////////
    //подсветка аномалии

    //подсветка idle состояния
    ref_light m_pIdleLight;
    Fcolor m_IdleLightColor;
    float m_fIdleLightRange;
    float m_fIdleLightHeight;
    CLAItem* m_pIdleLAnim;

    void StartIdleLight();
    void StopIdleLight();
    void UpdateIdleLight();

    //подсветка выброса
    ref_light m_pLight;
    float m_fLightRange;
    Fcolor m_LightColor;
    float m_fLightTime;
    float m_fLightTimeLeft;
    float m_fLightHeight;

    void StartBlowoutLight();
    void StopBlowoutLight();
    void UpdateBlowoutLight();

    //список партиклов для объетов внутри зоны
    using OBJECT_INFO_VEC = xr_vector<SZoneObjectInfo>;
    OBJECT_INFO_VEC m_ObjectInfoMap;

    void CreateHit(u16 id_to, u16 id_from, const Fvector& hit_dir, float hit_power, s16 bone_id,
        const Fvector& pos_in_bone, float hit_impulse, ALife::EHitType hit_type);

    virtual void Hit(SHit* pHDS);

    //для визуализации зоны
    virtual void PlayIdleParticles(bool bIdleLight = true);
    virtual void StopIdleParticles(bool bIdleLight = true);
    void PlayAccumParticles();
    void PlayAwakingParticles();
    void PlayBlowoutParticles();
    void PlayEntranceParticles(CGameObject* pObject);
    void PlayBulletParticles(Fvector& pos);
    void PlayBoltEntranceParticles();

    void PlayHitParticles(CGameObject* pObject);

    void PlayObjectIdleParticles(CGameObject* pObject);
    void StopObjectIdleParticles(CGameObject* pObject);

    virtual bool IsVisibleForZones() { return false; }
    //обновление, если зона передвигается
    virtual void OnMove();
    Fvector m_vPrevPos;
    u32 m_dwLastTimeMoved;

    //расстояние от зоны до текущего актера
    float m_fDistanceToCurEntity;

protected:
    u32 m_ef_anomaly_type;
    u32 m_ef_weapon_type;

public:
    void CalcDistanceTo(const Fvector& P, float& dist, float& radius);
    virtual u32 ef_anomaly_type() const;
    virtual u32 ef_weapon_type() const;
    virtual bool register_schedule() const { return true; }
    // optimization FAST/SLOW mode
public:
    virtual BOOL AlwaysTheCrow();
    void o_switch_2_fast();
    void o_switch_2_slow();

    // Lain: adde
private:
    virtual bool light_in_slow_mode() { return true; }
};
