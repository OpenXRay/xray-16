
#ifndef CHARACTER_PHYSICS_SUPPORT
#define CHARACTER_PHYSICS_SUPPORT

#include "alife_space.h"
#include "PHSkeleton.h"
#include "entity_alive.h"
#include "PHSoundPlayer.h"
#include "PHDestroyable.h"
#include "character_hit_animations.h"
#include "death_anims.h"
#include "character_shell_control.h"
#include "animation_utils.h"
class CPhysicsShell;
class CPHMovementControl;
class CIKLimbsController;
class interactive_motion;
class interactive_animation;
class physics_shell_animated;
class CODEGeom;
class CPhysicsElement;
class activating_character_delay;

class CCharacterPhysicsSupport : public CPHSkeleton, public CPHDestroyable
{
public:
    enum EType
    {
        etActor,
        etStalker,
        etBitting
    };

    enum EState
    {
        esDead,
        esAlive,
        esRemoved
    };

private:
    EType m_eType;
    EState m_eState;
    Flags8 m_flags;
    enum Fags
    {
        fl_death_anim_on = 1 << 0,
        fl_skeleton_in_shell = 1 << 1,
        fl_specific_bonce_demager = 1 << 2,
        fl_block_hit = 1 << 3,
        fl_use_hit_anims = 1 << 4
    };

    struct animation_movement_state
    {
        bool active;
        bool character_exist;
        void init()
        {
            active = false;
            character_exist = false;
        }
        animation_movement_state() { init(); }
    } anim_mov_state;

    CEntityAlive& m_EntityAlife;
    Fmatrix& mXFORM;
    CPhysicsShell*& m_pPhysicsShell;
    CPhysicsShell* m_physics_skeleton;
    CPHMovementControl* m_PhysicMovementControl;
    CPHSoundPlayer m_ph_sound_player;
    CIKLimbsController* m_ik_controller;
    ICollisionHitCallback* m_collision_hit_callback;
    character_hit_animation_controller m_hit_animations;
    death_anims m_death_anims;
    float m_BonceDamageFactor;
    interactive_motion* m_interactive_motion;
    character_shell_control m_character_shell_control;
    interactive_animation* m_interactive_animation;
    physics_shell_animated* m_physics_shell_animated;
    activating_character_delay* m_collision_activating_delay;
    xr_vector<CODEGeom*> m_weapon_geoms;
    xr_vector<anim_bone_fix*> m_weapon_bone_fixes;
    CPhysicsElement* m_weapon_attach_bone;
    CPhysicsShellHolder* m_active_item_obj;
    SHit m_sv_hit;
    u32 m_hit_valide_time;
    u32 m_physics_shell_animated_time_destroy;

public:
    EType Type() { return m_eType; }
private:
    EState STate() { return m_eState; }
    void SetState(EState astate) { m_eState = astate; }
    IC bool isDead() { return m_eState == esDead; }
    IC bool isAlive() { return !m_pPhysicsShell; }
protected:
    virtual void SpawnInitPhysics(CSE_Abstract* D);
    virtual CPhysicsShellHolder* PPhysicsShellHolder() { return m_EntityAlife.PhysicsShellHolder(); }
    virtual bool CanRemoveObject();

public:
    IC CPHMovementControl* movement() { return m_PhysicMovementControl; }
    IC const CPHMovementControl* movement() const { return m_PhysicMovementControl; }
    IC CPHSoundPlayer* ph_sound_player() { return &m_ph_sound_player; }
    IC CIKLimbsController* ik_controller() { return m_ik_controller; }
    bool is_interactive_motion();
    bool can_drop_active_weapon();
    void SetRemoved();
    bool IsRemoved() { return m_eState == esRemoved; }
    bool IsSpecificDamager() { return !!m_flags.test(fl_specific_bonce_demager); }
    float BonceDamageFactor() { return m_BonceDamageFactor; }
    void set_movement_position(const Fvector& pos);
    void ForceTransform(const Fmatrix& m);
    void set_use_hit_anims(bool v) { m_flags.set(fl_use_hit_anims, (BOOL)v); }
    //////////////////base hierarchi methods///////////////////////////////////////////////////
    void CreateCharacterSafe();
    void CreateCharacter();
    bool CollisionCorrectObjPos();

    void in_UpdateCL();
    void in_shedule_Update(u32 DT);
    void in_NetSpawn(CSE_Abstract* e);
    void in_NetDestroy();
    void destroy_imotion();
    void in_NetRelcase(IGameObject* O);
    void in_Init();
    void in_Load(LPCSTR section);
    void in_Hit(SHit& H, bool is_killing = false);
    void in_NetSave(NET_Packet& P);
    void in_ChangeVisual();
    void in_Die();
    void on_create_anim_mov_ctrl();
    void on_destroy_anim_mov_ctrl();
    void PHGetLinearVell(Fvector& velocity);
    ICollisionHitCallback* get_collision_hit_callback();
    void set_collision_hit_callback(ICollisionHitCallback* cc);
    void run_interactive(CBlend* B);
    void update_interactive_anims();
    IC physics_shell_animated* animation_collision() { return m_physics_shell_animated; }
    IC const physics_shell_animated* animation_collision() const { return m_physics_shell_animated; }
    void create_animation_collision();
    void destroy_animation_collision();
    u16 PHGetSyncItemsNumber();
    CPHSynchronize* PHGetSyncItem(u16 item);

private:
    void update_animation_collision();

public:
    //		void							on_active_weapon_shell_activate();
    bool has_shell_collision_place(const CPhysicsShellHolder* obj) const;
    virtual void on_child_shell_activate(CPhysicsShellHolder* obj);
    /////////////////////////////////////////////////////////////////
    CCharacterPhysicsSupport& operator=(CCharacterPhysicsSupport& /**asup/**/)
    {
        R_ASSERT2(false, "Can not assign it");
    }
    CCharacterPhysicsSupport(EType atype, CEntityAlive* aentity);
    virtual ~CCharacterPhysicsSupport();

private:
    void CreateSkeleton(CPhysicsShell*& pShell);

    void ActivateShell(IGameObject* who);
    void CreateShell(IGameObject* who, Fvector& dp, Fvector& velocity);
    void AddActiveWeaponCollision();
    void RemoveActiveWeaponCollision();
    void bone_chain_disable(u16 bone, u16 r_bone, IKinematics& K);
    void bone_fix_clear();
    void EndActivateFreeShell(
        IGameObject* who, const Fvector& inital_entity_position, const Fvector& dp, const Fvector& velocity);
    void KillHit(SHit& H);
    static void DeathAnimCallback(CBlend* B);
    void CreateIKController();
    void DestroyIKController();
    bool CollisionCorrectObjPos(const Fvector& start_from, bool character_create = false);

    void FlyTo(const Fvector& disp);
    IC void UpdateDeathAnims();
    IC bool DoCharacterShellCollide();
    void UpdateCollisionActivatingDellay();
    void SpawnCharacterCreate();
};
#endif // CHARACTER_PHYSICS_SUPPORT
