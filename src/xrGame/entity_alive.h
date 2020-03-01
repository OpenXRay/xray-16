#pragma once

#include "Entity.h"

// Igor DEFINE_VECTOR(ref_shader, SHADER_VECTOR, SHADER_VECTOR_IT);
using STR_VECTOR = xr_vector<shared_str>;

class MONSTER_COMMUNITY;
class CEntityCondition;
class CWound;
class CCharacterPhysicsSupport;
class CMaterialManager;
class CVisualMemoryManager;
class CBlend;
class CEntityAlive : public CEntity
{
protected:
    using inherited = CEntity;

private:
    u32 m_used_time;

public:
    virtual CEntityAlive* cast_entity_alive() { return this; }

    bool m_bMobility;
    float m_fAccuracy;
    float m_fIntelligence;
    u32 m_use_timeout;
    u8 m_squad_index;

private:
    bool m_is_agresive;
    bool m_is_start_attack;
    // m_PhysicMovementControl
    // CPHMovementControl		*m_PhysicMovementControl;
public:
    // General
    CEntityAlive();
    virtual ~CEntityAlive();

    // Core events
    virtual IFactoryObject* _construct();
    virtual void Load(LPCSTR section);
    virtual void reinit();
    virtual void reload(LPCSTR section);

    // object serialization
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual BOOL net_SaveRelevant();

    virtual void shedule_Update(u32 dt);
    virtual void create_anim_mov_ctrl(CBlend* b, Fmatrix* start_pose, bool local_animation);
    virtual void destroy_anim_mov_ctrl();

    virtual void HitImpulse(float amount, Fvector& vWorldDir, Fvector& vLocalDir);
    virtual void Hit(SHit* pHDS);
    virtual void Die(IGameObject* who);
    virtual void g_WeaponBones(int& L, int& R1, int& R2) = 0;
    void set_lock_corpse(bool b_l_corpse);
    bool is_locked_corpse();
    //	virtual float			GetfHealth				() const;
    //	virtual float			SetfHealth				(float value);

    //	virtual float			g_Health				()	const;
    //	virtual float			g_MaxHealth				()	const;

    virtual float g_Radiation() const;
    virtual float SetfRadiation(float value);

    virtual float CalcCondition(float hit);

    // Visibility related
    virtual float ffGetFov() const = 0;
    virtual float ffGetRange() const = 0;

    virtual bool human_being() const { return false; }
public:
    // IC	CPHMovementControl*					PMovement					()						{return
    // m_PhysicMovementControl;}

    virtual u16 PHGetSyncItemsNumber();
    virtual CPHSynchronize* PHGetSyncItem(u16 item);
    virtual void PHUnFreeze();
    virtual void PHFreeze();

    virtual void PHGetLinearVell(Fvector& velocity);
    virtual CPHSoundPlayer* ph_sound_player();
    virtual CIKLimbsController* character_ik_controller();
    virtual ICollisionHitCallback* get_collision_hit_callback();
    virtual void set_collision_hit_callback(ICollisionHitCallback* cc);

protected:
    using WOUND_VECTOR = xr_vector<CWound*>;
    WOUND_VECTOR m_ParticleWounds;

    virtual void StartFireParticles(CWound* pWound);
    virtual void UpdateFireParticles();
    virtual void LoadFireParticles(LPCSTR section);

public:
    static void UnloadFireParticles();

protected:
    static STR_VECTOR* m_pFireParticlesVector;
    static u32 m_dwMinBurnTime;
    static float m_fStartBurnWoundSize;
    static float m_fStopBurnWoundSize;

    virtual void BloodyWallmarks(float P, const Fvector& dir, s16 element, const Fvector& position_in_object_space);
    static void LoadBloodyWallmarks(LPCSTR section);

public:
    static void UnloadBloodyWallmarks();

    void ClearBloodWounds() { m_BloodWounds.clear(); };
protected:
    virtual void PlaceBloodWallmark(const Fvector& dir, const Fvector& start_pos, float trace_dist, float wallmark_size,
        IWallMarkArray* pwallmarks_vector);

    //информация о кровавых отметках на стенах, общая для всех CEntityAlive
    static FactoryPtr<IWallMarkArray>* m_pBloodMarksVector;
    static float m_fBloodMarkSizeMax;
    static float m_fBloodMarkSizeMin;
    static float m_fBloodMarkDistance;
    static float m_fNominalHit;

    //текстурки капель крови
    static FactoryPtr<IWallMarkArray>* m_pBloodDropsVector;
    //список ран с которых капает кровь

    WOUND_VECTOR m_BloodWounds;
    //размер раны, чтоб начала капать кровь
    static float m_fStartBloodWoundSize;
    //размер раны, чтоб остановить кровь
    static float m_fStopBloodWoundSize;
    static float m_fBloodDropSize;

    //обновление ран, и рисование отметок от капающей крови
    virtual void StartBloodDrops(CWound* pWound);
    virtual void UpdateBloodDrops();

    //отношения между существами и персонажами в зоне
public:
    virtual ALife::ERelationType tfGetRelationType(const CEntityAlive* tpEntityAlive) const;
    virtual bool is_relation_enemy(const CEntityAlive* tpEntityAlive) const;

    MONSTER_COMMUNITY* monster_community;

private:
    CEntityCondition* m_entity_condition;
    CMaterialManager* m_material_manager;
    bool b_eating;

protected:
    virtual CEntityConditionSimple* create_entity_condition(CEntityConditionSimple* ec);

public:
    IC CEntityCondition& conditions() const;
    IC CMaterialManager& material() const
    {
        VERIFY(m_material_manager);
        return (*m_material_manager);
    }

protected:
    u32 m_ef_creature_type;
    u32 m_ef_weapon_type;
    u32 m_ef_detector_type;

public:
    virtual u32 ef_creature_type() const;
    virtual u32 ef_weapon_type() const;
    virtual u32 ef_detector_type() const;

    virtual void OnHitHealthLoss(float NewHealth){}; //вызывается если entity теряет здоровье
    virtual void OnCriticalHitHealthLoss(){}; //вызывается если entity умрет от хита
    virtual void OnCriticalWoundHealthLoss(){}; //вызывается если entity умрет от потери крови
    virtual void OnCriticalRadiationHealthLoss(){}; //вызывается если entity умрет от радиации

    virtual CVisualMemoryManager* visual_memory() const { return (0); }
    virtual void net_Relcase(IGameObject* O);

    virtual Fvector predict_position(const float& time_to_check) const;
    virtual Fvector target_position() const;
    IC bool const& is_agresive() const;
    IC void is_agresive(bool const& value);
    IC bool const& is_start_attack() const;
    IC void is_start_attack(bool const& value);

    virtual Fvector get_new_local_point_on_mesh(u16& bone_id) const;
    virtual Fvector get_last_local_point_on_mesh(Fvector const& last_point, u16 bone_id) const;
    virtual void OnChangeVisual();

private:
    void fill_hit_bone_surface_areas() const;

    typedef xr_vector<std::pair<u16, float>> hit_bone_surface_areas_type;

    mutable hit_bone_surface_areas_type m_hit_bone_surface_areas;
    mutable CRandom m_hit_bones_random;
    mutable bool m_hit_bone_surface_areas_actual;
};

#include "entity_alive_inline.h"
