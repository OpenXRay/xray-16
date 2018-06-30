#pragma once

#include "gameobject.h"
#include "physicsshellholder.h"
#include "physicsskeletonobject.h"
#include "PHSkeleton.h"
#include "animation_script_callback.h"
#include "xrserver_objects_alife.h"
#include "xrCommon/xr_deque.h"

class CSE_ALifeObjectPhysic;
class CPhysicsElement;
class moving_bones_snd_player;

class CSE_ALifeObjectPhysic;
struct SPHNetState;
typedef CSE_ALifeObjectPhysic::mask_num_items mask_num_items;

struct net_update_PItem
{
    u32 dwTimeStamp;
    SPHNetState State;
};

struct net_updatePhData
{
    xr_deque<net_update_PItem> NET_IItem;
    /// spline coeff /////////////////////
    // float			SCoeff[3][4];
    /*Fvector			IStartPos;
    Fquaternion		IStartRot;

    Fvector			IRecPos;
    Fquaternion		IRecRot;

    Fvector			IEndPos;
    Fquaternion		IEndRot;	*/

    //	SPHNetState		LastState;
    //	SPHNetState		RecalculatedState;

    //	SPHNetState		PredictedState;

    u32 m_dwIStartTime;
    u32 m_dwIEndTime;
    // u32				m_dwILastUpdateTime;
};

class CPhysicObject : public CPhysicsShellHolder, public CPHSkeleton
{
protected:
    using inherited = CPhysicsShellHolder;

private:
    EPOType m_type;
    float m_mass;
    ICollisionHitCallback* m_collision_hit_callback;
    CBlend* m_anim_blend;
    moving_bones_snd_player* bones_snd_player;
    anim_script_callback m_anim_script_callback;

private:
    // Creating
    void CreateBody(CSE_ALifeObjectPhysic* po);
    void CreateSkeleton(CSE_ALifeObjectPhysic* po);
    void AddElement(CPhysicsElement* root_e, int id);
    void create_collision_model();

public:
    void run_anim_forward();
    void run_anim_back();
    void stop_anim();
    void play_bones_sound();
    void stop_bones_sound();
    float anim_time_get();
    void anim_time_set(float time);
    void set_door_ignore_dynamics();
    void unset_door_ignore_dynamics();
    bool get_door_vectors(Fvector& closed, Fvector& open) const;

public:
    CPhysicObject(void);
    virtual ~CPhysicObject(void);
    // virtual void						make_Interpolation	(); // interpolation from last visible to corrected
    // position/rotation
    virtual void Interpolate();
    float interpolate_states(net_update_PItem const& first, net_update_PItem const& last, SPHNetState& current);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void CreatePhysicsShell(CSE_Abstract* e);
    virtual void net_Destroy();
    virtual void Load(LPCSTR section);
    virtual void shedule_Update(u32 dt); //
    virtual void UpdateCL();
    virtual void net_Save(NET_Packet& P);
    virtual BOOL net_SaveRelevant();
    virtual BOOL UsedAI_Locations();
    virtual ICollisionHitCallback* get_collision_hit_callback();
    virtual void set_collision_hit_callback(ICollisionHitCallback* cc);
    virtual bool is_ai_obstacle() const;

    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);

    virtual void PH_B_CrPr(); // actions & operations before physic correction-prediction steps
    virtual void PH_I_CrPr(); // actions & operations after correction before prediction steps
    virtual void PH_A_CrPr(); // actions & operations after phisic correction-prediction steps
protected:
    virtual void SpawnInitPhysics(CSE_Abstract* D);
    virtual void RunStartupAnim(CSE_Abstract* D);
    virtual CPhysicsShellHolder* PPhysicsShellHolder() { return PhysicsShellHolder(); }
    virtual CPHSkeleton* PHSkeleton() { return this; }
    virtual void InitServerObject(CSE_Abstract* po);
    virtual void PHObjectPositionUpdate();

    void net_Export_PH_Params(NET_Packet& P, SPHNetState& State, mask_num_items& num_items);
    void net_Import_PH_Params(NET_Packet& P, net_update_PItem& N, mask_num_items& num_items);
    net_updatePhData* NetSync();
    net_updatePhData* m_net_updateData;
    void CalculateInterpolationParams();

    enum EIIFlags
    {
        Fdrop = (1 << 0),
        FCanTake = (1 << 1),
        FCanTrade = (1 << 2),
        Fbelt = (1 << 3),
        Fruck = (1 << 4),
        FRuckDefault = (1 << 5),
        FUsingCondition = (1 << 6),
        FAllowSprint = (1 << 7),
        Fuseful_for_NPC = (1 << 8),
        FInInterpolation = (1 << 9),
        FInInterpolate = (1 << 10),
        FIsQuestItem = (1 << 11),
    };
    Flags16 m_flags;
    bool m_just_after_spawn;
    bool m_activated;
};
