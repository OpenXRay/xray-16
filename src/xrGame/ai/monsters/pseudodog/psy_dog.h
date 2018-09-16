#pragma once
#include "pseudodog.h"

class CPsyDogPhantom;
class CPsyDogAura;

class CPsyDog : public CAI_PseudoDog
{
    typedef CAI_PseudoDog inherited;

    friend class CPsyDogPhantom;
    friend class CPsyDogAura;

    // эффектор у актера при нахождении в поле
    CPsyDogAura* m_aura;

    // enemy transfered from phantom
    CActor* m_enemy;

    // externals
    u8 m_max_phantoms_count;
    u32 m_time_phantom_respawn;

    static u32 const s_phantom_immediate_respawn_flag = 0;
    static u32 const s_phantom_alive_flag = 1;

    TTime* m_phantoms_die_time;

public:
    CPsyDog();
    virtual ~CPsyDog();

    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* dc);
    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual void net_Destroy();
    virtual void Die(IGameObject* who);

    virtual void Think();
    //				void	on_phantom_appear	();
    virtual IStateManagerBase* create_state_manager();

    pcstr get_monster_class_name() override { return "psydog"; }
    u8 get_phantoms_count();
    bool must_hide() { return get_phantoms_count() == 0; }
private:
    bool spawn_phantom();
    void delete_phantom(CPsyDogPhantom*);
    void register_phantom(CPsyDogPhantom*);
    void unregister_phantom(CPsyDogPhantom*);

    void delete_all_phantoms();

private:
    xr_vector<CPsyDogPhantom*> m_storage;
};

//////////////////////////////////////////////////////////////////////////
// Phantom Psy Dog
//////////////////////////////////////////////////////////////////////////

class CPsyDogPhantom : public CAI_PseudoDog
{
    typedef CAI_PseudoDog inherited;

    CPsyDog* m_parent;

    enum
    {
        eWaitToAppear,
        eAttack
    } m_state;

    SAttackEffector m_appear_effector;

    LPCSTR m_particles_appear;
    LPCSTR m_particles_disappear;

    u16 m_parent_id;

    u32 m_time_spawned;

public:
    CPsyDogPhantom();
    virtual ~CPsyDogPhantom();
    virtual BOOL net_Spawn(CSE_Abstract* dc);
    virtual void Think();
    virtual void Hit(SHit* pHDS);

    virtual void net_Destroy();
    virtual void Die(IGameObject* who);

    void destroy_from_parent();

private:
    void destroy_me();
    void try_to_register_to_parent();
    bool is_wait_to_destroy_object() { return (m_parent_id == 0xffff); }
};
