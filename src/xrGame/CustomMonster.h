// CustomMonster.h: interface for the CCustomMonster class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "entity_alive.h"
#include "script_entity.h"
#include "xrEngine/Feel_Vision.h"
#include "xrEngine/Feel_Sound.h"
#include "xrEngine/Feel_Touch.h"
#include "Include/xrRender/Kinematics.h"
#include "xrCore/Containers/AssociativeVector.hpp"
#include "trajectories.h"

namespace MonsterSpace
{
struct SBoneRotation;
};

class CMotionDef;
class IKinematicsAnimated;
class CMemoryManager;
class CItemManager;
class CEnemyManager;
class CDangerManager;
class CMovementManager;
class CSoundPlayer;
class CAI_Stalker;
class CDangerObject;
class moving_object;

class CCustomMonster : public CEntityAlive,
                       public CScriptEntity,
                       public Feel::Vision,
                       public Feel::Sound,
                       public Feel::Touch
{
protected:
    typedef CEntityAlive inherited;

private:
    CMemoryManager* m_memory_manager;
    CMovementManager* m_movement_manager;
    CSoundPlayer* m_sound_player;

private:
    u32 m_client_update_delta;
    u32 m_last_client_update_time;

protected:
    struct SAnimState
    {
        MotionID fwd;
        MotionID back;
        MotionID ls;
        MotionID rs;

        void Create(IKinematicsAnimated* K, LPCSTR base);
    };

private:
    xr_vector<CLASS_ID> m_killer_clsids;

public:
    // Eyes
    Fmatrix eye_matrix;
    int eye_bone;
    float eye_fov;
    float eye_range;

    float m_fCurSpeed;

    u32 eye_pp_stage;
    u32 eye_pp_timestamp;
    Fvector m_tEyeShift;
    float m_fEyeShiftYaw;
    BOOL NET_WasExtrapolating;

    Fvector tWatchDirection;

    virtual void Think() = 0;

    float m_fTimeUpdateDelta;
    u32 m_dwLastUpdateTime;
    //	Fmatrix				m_tServerTransform;

    u32 m_dwCurrentTime; // time updated in UpdateCL

    struct net_update
    {
        u32 dwTimeStamp; // server(game) timestamp
        float o_model; // model yaw
        SRotation o_torso; // torso in world coords
        Fvector p_pos; // in world coords
        float fHealth;

        // non-exported (temporal)

        net_update()
        {
            dwTimeStamp = 0;
            o_model = 0;
            o_torso.yaw = 0;
            o_torso.pitch = 0;
            p_pos.set(0, 0, 0);
            fHealth = 0.f;
        }
        void lerp(net_update& A, net_update& B, float f);
    };
    xr_deque<net_update> NET;
    net_update NET_Last;
    BOOL NET_WasInterpolating; // previous update was by interpolation or by extrapolation
    u32 NET_Time; // server time of last update
    //------------------------------

    virtual bool feel_touch_on_contact(IGameObject*);
    virtual bool feel_touch_contact(IGameObject*);
    // utils
    void mk_orientation(Fvector& dir, Fmatrix& mR);
    void mk_rotation(Fvector& dir, SRotation& R);

    // stream executors
    virtual void Exec_Action(float dt);
    virtual void Exec_Look(float dt);
    void __stdcall Exec_Visibility();
    virtual void eye_pp_s0();
    void eye_pp_s1();
    void eye_pp_s2();

    virtual void UpdateCamera();

public:
    CCustomMonster();
    virtual ~CCustomMonster();

public:
    virtual CEntityAlive* cast_entity_alive() { return this; }
    virtual CEntity* cast_entity() { return this; }
public:
    virtual IFactoryObject* _construct();
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void Die(IGameObject* who);

    virtual void HitSignal(float P, Fvector& vLocalDir, IGameObject* who);
    virtual void g_WeaponBones(int& /**L/**/, int& /**R1/**/, int& /**R2/**/){};
    virtual void shedule_Update(u32 DT);
    virtual void UpdateCL();

    // Network
    virtual void net_Export(NET_Packet& P); // export to server
    virtual void net_Import(NET_Packet& P); // import from server
    virtual void net_Relcase(IGameObject* O);

    virtual void SelectAnimation(const Fvector& _view, const Fvector& _move, float speed) = 0;

    virtual bool is_base_monster_with_enemy() { return false; }
// debug
#ifdef DEBUG
    virtual void OnRender();
    virtual void OnHUDDraw(CCustomHUD* hud);
#endif

    virtual bool bfExecMovement() { return (false); };
    IC bool angle_lerp_bounds(float& a, float b, float c, float d);
    IC void vfNormalizeSafe(Fvector& Vector);

public:
    virtual float ffGetFov() const { return eye_fov; }
    virtual float ffGetRange() const { return eye_range; }
    void set_fov(float new_fov);
    void set_range(float new_range);
    //	virtual	void				feel_touch_new			(IGameObject	*O);
    virtual BOOL feel_visible_isRelevant(IGameObject* O);
    virtual Feel::Sound* dcast_FeelSound() { return this; }
    virtual void Hit(SHit* pHDS);

    virtual void OnEvent(NET_Packet& P, u16 type);
    virtual void net_Destroy();
    virtual BOOL UsedAI_Locations();
    ///////////////////////////////////////////////////////////////////////
    virtual u16 PHGetSyncItemsNumber() { return inherited::PHGetSyncItemsNumber(); }
    virtual CPHSynchronize* PHGetSyncItem(u16 item) { return inherited::PHGetSyncItem(item); }
    virtual void PHUnFreeze() { return inherited::PHUnFreeze(); }
    virtual void PHFreeze() { return inherited::PHFreeze(); }
    ///////////////////////////////////////////////////////////////////////
public:
    virtual void Load(LPCSTR section);
    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual const SRotation Orientation() const;
    virtual float get_custom_pitch_speed(float def_speed) { return def_speed; }
    virtual bool human_being() const { return (false); }
    virtual void PitchCorrection();

    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);
    virtual BOOL net_SaveRelevant() { return inherited::net_SaveRelevant(); }
    virtual const MonsterSpace::SBoneRotation& head_orientation() const;

    virtual void UpdatePositionAnimation();
    virtual void set_ready_to_save();
    virtual CPhysicsShellHolder* cast_physics_shell_holder() { return this; }
    virtual CParticlesPlayer* cast_particles_player() { return this; }
    virtual CCustomMonster* cast_custom_monster() { return this; }
    virtual CScriptEntity* cast_script_entity() { return this; }
    void load_killer_clsids(LPCSTR section);
    bool is_special_killer(IGameObject* obj);

    IC CMemoryManager& memory() const;
    virtual float feel_vision_mtl_transp(IGameObject* O, u32 element);
    virtual void feel_sound_new(
        IGameObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position, float power);

    virtual bool useful(const CItemManager* manager, const CGameObject* object) const;
    virtual float evaluate(const CItemManager* manager, const CGameObject* object) const;
    virtual bool useful(const CEnemyManager* manager, const CEntityAlive* object) const;
    virtual float evaluate(const CEnemyManager* manager, const CEntityAlive* object) const;
    virtual bool useful(const CDangerManager* manager, const CDangerObject& object) const;
    virtual float evaluate(const CDangerManager* manager, const CDangerObject& object) const;

protected:
    float m_panic_threshold;

public:
    IC float panic_threshold() const;

private:
    CSound_UserDataVisitor* m_sound_user_data_visitor;

protected:
    virtual CSound_UserDataVisitor* create_sound_visitor();
    virtual CMemoryManager* create_memory_manager();
    virtual CMovementManager* create_movement_manager();

public:
    IC CMovementManager& movement() const;
    IC CSoundPlayer& sound() const;
    IC CSound_UserDataVisitor* sound_user_data_visitor() const;

protected:
    float m_far_plane_factor;
    float m_fog_density_factor;

public:
    virtual void update_range_fov(float& new_range, float& new_fov, float start_range, float start_fov);

public:
    void __stdcall update_sound_player();
    virtual void on_restrictions_change();

    virtual bool should_wait_to_use_corspe_visual() { return true; }
    virtual LPCSTR visual_name(CSE_Abstract* server_entity);

private:
    bool m_already_dead;

public:
    IC const bool& already_dead() const { return (m_already_dead); };
    virtual bool use_simplified_visual() const { return false; } //(already_dead());};
    virtual void on_enemy_change(const CEntityAlive* enemy);
    virtual CVisualMemoryManager* visual_memory() const;

public:
    IC float client_update_fdelta() const;
    IC const u32& client_update_delta() const;
    IC const u32& last_client_update_time() const;

    //////////////////////////////////////////////////////////////////////////
    // Critical Wounds
    //////////////////////////////////////////////////////////////////////////
public:
    typedef u32 CriticalWoundType;

private:
    typedef AssociativeVector<u16, CriticalWoundType> BODY_PART;

protected:
    u32 m_last_hit_time;
    float m_critical_wound_threshold;
    float m_critical_wound_decrease_quant;
    float m_critical_wound_accumulator;
    CriticalWoundType m_critical_wound_type;
    BODY_PART m_bones_body_parts;

protected:
    virtual void load_critical_wound_bones() {}
    virtual bool critical_wound_external_conditions_suitable() { return true; }
    virtual void critical_wounded_state_start() {}
    bool update_critical_wounded(const u16& bone_id, const float& power);

public:
    IC void critical_wounded_state_stop();

public:
    IC bool critically_wounded();
    IC const u32& critical_wound_type() const;

    //////////////////////////////////////////////////////////////////////////
private:
    bool m_invulnerable;

public:
    IC void invulnerable(const bool& invulnerable);
    IC bool invulnerable() const;

private:
    moving_object* m_moving_object;

public:
    IC moving_object* get_moving_object() const;
    virtual void spatial_move();
    virtual Fvector predict_position(const float& time_to_check) const;
    virtual Fvector target_position() const;

protected:
    bool m_update_rotation_on_frame;

private:
    bool m_movement_enabled_before_animation_controller;

public:
    virtual void create_anim_mov_ctrl(CBlend* b, Fmatrix* start_pose, bool local_animation);
    virtual void destroy_anim_mov_ctrl();
    virtual void ForceTransform(Fmatrix const& m);

public:
    virtual Fvector spatial_sector_point();

#ifdef DEBUG
    xr_vector<trajectory_pick> m_jump_picks;
    xr_vector<Fvector> m_jump_collide_tris;
#endif // #ifdef DEBUG
};

#include "CustomMonster_inline.h"
