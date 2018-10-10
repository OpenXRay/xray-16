////////////////////////////////////////////////////////////////////////////
//	Module 		: memory_manager.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Memory manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "enemy_manager.h"

class CVisualMemoryManager;
class CSoundMemoryManager;
class CHitMemoryManager;
class CItemManager;
class CDangerManager;
class CCustomMonster;
class CAI_Stalker;
class CEntityAlive;
class CSound_UserDataVisitor;

namespace MemorySpace
{
struct CMemoryInfo;
};

class CMemoryManager
{
public:
    typedef MemorySpace::CMemoryInfo CMemoryInfo;

protected:
    CVisualMemoryManager* m_visual;
    CSoundMemoryManager* m_sound;
    CHitMemoryManager* m_hit;
    CEnemyManager* m_enemy;
    CItemManager* m_item;
    CDangerManager* m_danger;

protected:
    CCustomMonster* m_object;
    CAI_Stalker* m_stalker;

private:
    void update_enemies(const bool& registered_in_combat);

protected:
    template <typename T>
    void update(const xr_vector<T>& objects, bool add_enemies);

public:
    CMemoryManager(CEntityAlive* entity_alive, CSound_UserDataVisitor* visitor);
    virtual ~CMemoryManager();
    virtual void Load(LPCSTR section);
    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual void update(float time_delta);
    void remove_links(IGameObject* object);
    virtual void on_restrictions_change();

public:
    void enable(const IGameObject* object, bool enable);
    CMemoryInfo memory(const IGameObject* object) const;
    u32 memory_time(const IGameObject* object) const;
    Fvector memory_position(const IGameObject* object) const;
    void make_object_visible_somewhen(const CEntityAlive* enemy);

public:
    template <typename T, typename _predicate>
    IC void fill_enemies(const xr_vector<T>& objects, const _predicate& predicate) const;
    template <typename _predicate>
    IC void fill_enemies(const _predicate& predicate) const;

public:
    IC CVisualMemoryManager& visual() const;
    IC CSoundMemoryManager& sound() const;
    IC CHitMemoryManager& hit() const;
    IC CEnemyManager& enemy() const;
    IC CItemManager& item() const;
    IC CDangerManager& danger() const;
    IC CCustomMonster& object() const;
    IC CAI_Stalker& stalker() const;

public:
    void save(NET_Packet& packet) const;
    void load(IReader& packet);
    void xr_stdcall on_requested_spawn(IGameObject* object);
};

#include "memory_manager_inline.h"
