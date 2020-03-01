// Level_Bullet_Manager.h:  для обеспечения полета пули по траектории
//							все пули и осколки передаются сюда
//////////////////////////////////////////////////////////////////////

#pragma once

#include "WeaponAmmo.h"
#include "Tracer.h"
#include "xrCDB/xr_collide_defs.h"
#include "xrCommon/xr_vector.h"
#include "xrSound/Sound.h"

//коэфициенты и параметры патрона
struct SBullet_Hit
{
    float power; // power          * cartridge
    float impulse; // impulse        * cartridge
};

//структура, описывающая пулю и ее свойства в полете
struct SBullet
{
    u32 init_frame_num; //номер кадра на котором была запущена пуля
    union
    {
        struct
        {
            u16 ricochet_was : 1; //пуля срикошетила
            u16 explosive : 1; // special explosive mode for particles
            u16 allow_tracer : 1;
            u16 allow_ricochet : 1; //разрешить рикошет
            u16 allow_sendhit : 1; // statistics
            //.			u16			skipped_frame	: 1	;			//пропуск первой отрисовки
            u16 aim_bullet : 1; //прицеленная пуля( вылетевшая первой после длительного молчания оружия (1-3 сек.))
            u16 magnetic_beam : 1; //магнитный луч (нет отклонения после пробивания, не падает скорость после
                                   //пробивания)
        };
        u16 _storage;
    } flags;
    u16 bullet_material_idx;

    Fvector bullet_pos; //текущая позиция
    Fvector dir;
    float speed; //текущая скорость

    u16 parent_id; // ID персонажа который иницировал действие
    u16 weapon_id; // ID оружия из которого была выпущены пуля

    float fly_dist; //дистанция которую пуля пролетела
    Fvector tracer_start_position;

    Fvector start_position;
    Fvector start_velocity;
    u32 born_time;
    float life_time;
    u32 change_rajectory_count;

    //коэфициенты и параметры патрона
    SBullet_Hit hit_param;
    //-------------------------------------------------------------------
    float air_resistance;
    //-------------------------------------------------------------------
    float max_speed; // maxspeed*cartridge
    float max_dist; // maxdist*cartridge
    float armor_piercing; // ap
    float wallmark_size;
    //-------------------------------------------------------------------
    u8 m_u8ColorID;

    //тип наносимого хита
    ALife::EHitType hit_type;
    //---------------------------------
    u32 m_dwID;
    ref_sound m_whine_snd;
    ref_sound m_mtl_snd;
    //---------------------------------
    u16 targetID;
    //---------------------------------
    bool density_mode;
    float density;
    Fvector begin_density;
    bool operator==(u32 ID) { return ID == m_dwID; }
public:
    SBullet();
    ~SBullet();

    bool CanBeRenderedNow() const { return (Device.dwFrame > init_frame_num); }
    void Init(const Fvector& position, const Fvector& direction, float start_speed, float power,
        //.										float	power_critical,
        float impulse, u16 sender_id, u16 sendersweapon_id, ALife::EHitType e_hit_type, float maximum_distance,
        const CCartridge& cartridge, float const air_resistance_factor, bool SendHit);
};

class CLevel;

class CBulletManager
{
    static float const parent_ignore_distance;

    collide::rq_results rq_storage;
    collide::rq_results m_rq_results;

    using SoundVec = xr_vector<ref_sound>;
    using BulletVec = xr_vector<SBullet>;
    friend CLevel;

    enum EventType
    {
        EVENT_HIT = u8(0),
        EVENT_REMOVE,

        EVENT_DUMMY = u8(-1),
    };
    struct _event
    {
        EventType Type;
        BOOL dynamic;
        BOOL Repeated; // последовательное повторное попадание в динамический объект
        SBullet_Hit hit_result;
        SBullet bullet;
        Fvector normal;
        Fvector point;
        collide::rq_result R;
        u16 tgt_material;
    };
    static void CalculateNewVelocity(Fvector& dest_new_vel, Fvector const& old_velocity, float ar, float life_time);

protected:
    SoundVec m_WhineSounds;
    RStringVec m_ExplodeParticles;

    //список пуль находящихся в данный момент на уровне
    //.	Lock		m_Lock				;

    BulletVec m_Bullets; // working set, locked
    BulletVec m_BulletsRendered; // copy for rendering
    xr_vector<_event> m_Events;

#ifdef DEBUG
    tid_t m_thread_id;

    typedef xr_vector<Fvector> BulletPoints;
    BulletPoints m_bullet_points;
#endif // #ifdef DEBUG

    //отрисовка трассеров от пуль
    CTracer tracers;

    //минимальная скорость, на которой пуля еще считается
    static float m_fMinBulletSpeed;

    float m_fHPMaxDist;

    //константа G
    float m_fGravityConst;
    //сопротивление воздуха, процент, который отнимается от скорости
    //полета пули
    float m_fAirResistanceK;
    // cколько процентов энергии потеряет пуля при столкновении с материалом (при падении под прямым углом)
    float m_fCollisionEnergyMin;
    //сколькол процентов энергии устанется у пули при любом столкновении
    float m_fCollisionEnergyMax;

    //параметры отрисовки трассеров
    float m_fTracerWidth;
    float m_fTracerLengthMax;
    float m_fTracerLengthMin;

protected:
    void PlayWhineSound(SBullet* bullet, IGameObject* object, const Fvector& pos);
    void PlayExplodePS(const Fmatrix& xf);
    //функция обработки хитов объектов
    static BOOL test_callback(const collide::ray_defs& rd, IGameObject* object, LPVOID params);
    static BOOL firetrace_callback(collide::rq_result& result, LPVOID params);

    // Deffer event
    void RegisterEvent(EventType Type, BOOL _dynamic, SBullet* bullet, const Fvector& end_point, collide::rq_result& R,
        u16 target_material);

    //попадание по динамическому объекту
    void DynamicObjectHit(_event& E);

    //попадание по статическому объекту
    void StaticObjectHit(_event& E);

    //попадание по любому объекту, на выходе - импульс и сила переданные пулей объекту
    bool ObjectHit(SBullet_Hit* hit_res, SBullet* bullet, const Fvector& end_point, collide::rq_result& R,
        u16 target_material, Fvector& hit_normal);
    //отметка на пораженном объекте
    void FireShotmark(SBullet* bullet, const Fvector& vDir, const Fvector& vEnd, collide::rq_result& R,
        u16 target_material, const Fvector& vNormal, bool ShowMark = true);
    //просчет полета пули за некоторый промежуток времени
    //принимается что на этом участке пуля движется прямолинейно
    //и равномерно, а после просчета также изменяется текущая
    //скорость и положение с учетом гравитации и ветра
    //возвращаем true если пуля продолжает полет
    bool trajectory_check_error(Fvector& previous_position, collide::rq_results& rq_storage, SBullet& bullet,
        float& low, float& high, Fvector const& gravity, float const air_resistance);
    void add_bullet_point(Fvector const& start_position, Fvector& previous_position, Fvector const& start_velocity,
        Fvector const& gravity, float const ait_resistance, float const current_time);
    bool process_bullet(collide::rq_results& rq_storage, SBullet& bullet, float delta_time);
    void __stdcall UpdateWorkload();

public:
    CBulletManager();
    virtual ~CBulletManager();

    void Load();
    void Clear();
    void AddBullet(const Fvector& position, const Fvector& direction, float starting_speed, float power,
        /*float power_critical,*/ float impulse, u16 sender_id, u16 sendersweapon_id, ALife::EHitType e_hit_type,
        float maximum_distance, const CCartridge& cartridge, float const air_resistance_factor, bool SendHit,
        bool AimBullet = false);

    void CommitEvents(); // @ the start of frame
    void CommitRenderSet(); // @ the end of frame
    void Render();
};

struct bullet_test_callback_data
{
    Fvector collide_position;
    SBullet* pBullet;
    float collide_time;
#if 1 // def DEBUG
    float high_time;
#endif // #ifdef DEBUG
};
