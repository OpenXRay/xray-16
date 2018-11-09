////////////////////////////////////////////////////////////////////////////
//	Module 		: visual_memory_manager.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Visual memory manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "visual_memory_params.h"
#include "memory_space.h"
#include "memory_space_impl.h"
#include "xrCommon/xr_vector.h"


class CCustomMonster;
class CAI_Stalker;
class vision_client;
class IGameObject;
class IReader;
class NET_Packet;

class CVisualMemoryManager
{
#ifdef DEBUG
    friend class CAI_Stalker;
#endif
public:
    typedef MemorySpace::CVisibleObject CVisibleObject;
    typedef MemorySpace::CNotYetVisibleObject CNotYetVisibleObject;
    typedef xr_vector<CVisibleObject> VISIBLES;
    typedef xr_vector<IGameObject*> RAW_VISIBLES;
    typedef xr_vector<CNotYetVisibleObject> NOT_YET_VISIBLES;

private:
    struct CDelayedVisibleObject
    {
        ALife::_OBJECT_ID m_object_id;
        CVisibleObject m_visible_object;
    };

private:
    typedef xr_vector<CDelayedVisibleObject> DELAYED_VISIBLE_OBJECTS;

private:
    CCustomMonster* m_object;
    CAI_Stalker* m_stalker;
    vision_client* m_client;

private:
    RAW_VISIBLES m_visible_objects;
    VISIBLES* m_objects;
    NOT_YET_VISIBLES m_not_yet_visible_objects;

private:
    DELAYED_VISIBLE_OBJECTS m_delayed_objects;

private:
    CVisionParameters m_free;
    CVisionParameters m_danger;

private:
    u32 m_max_object_count;
    bool m_enabled;
    u32 m_last_update_time;

public:
    void add_visible_object(const IGameObject* object, float time_delta, bool fictitious = false);

protected:
    IC void fill_object(CVisibleObject& visible_object, const CGameObject* game_object);
    bool should_ignore_object(IGameObject const* object) const;
    void add_visible_object(const CVisibleObject visible_object);
    float object_visible_distance(const CGameObject* game_object, float& object_distance) const;
    float object_luminocity(const CGameObject* game_object) const;
    float get_visible_value(
        float distance, float object_distance, float time_delta, float object_velocity, float luminocity) const;
    float get_object_velocity(const CGameObject* game_object, const CNotYetVisibleObject& not_yet_visible_object) const;
    u32 get_prev_time(const CGameObject* game_object) const;

public:
    u32 visible_object_time_last_seen(const IGameObject* object) const;

protected:
    void add_not_yet_visible_object(const CNotYetVisibleObject& not_yet_visible_object);
    CNotYetVisibleObject* not_yet_visible_object(const CGameObject* game_object);

private:
    void initialize();

public:
    CVisualMemoryManager(CCustomMonster* object);
    CVisualMemoryManager(CAI_Stalker* stalker);
    CVisualMemoryManager(vision_client* client);
    virtual ~CVisualMemoryManager();
    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual void update(float time_delta);
    virtual float feel_vision_mtl_transp(IGameObject* O, u32 element);
    void remove_links(IGameObject* object);

public:
    bool visible(const CGameObject* game_object, float time_delta);
    bool visible(u32 level_vertex_id, float yaw, float eye_fov) const;

public:
    IC void set_squad_objects(xr_vector<CVisibleObject>* squad_objects);
    CVisibleObject* visible_object(const CGameObject* game_object);

public:
    // this function returns true if and only if
    // specified object is visible now
    bool visible_right_now(const CGameObject* game_object) const;
    // if current_params.m_still_visible_time == 0
    // this function returns true if and only if
    // specified object is visible now
    // if current_params.m_still_visible_time > 0
    // this function returns true if and only if
    // specified object is visible now or
    // some time ago <= current_params.m_still_visible_time
    bool visible_now(const CGameObject* game_object) const;

public:
    void enable(const IGameObject* object, bool enable);

public:
    IC float visibility_threshold() const;
    IC float transparency_threshold() const;

public:
    IC bool enabled() const;
    IC void enable(bool value);

public:
    IC const VISIBLES& objects() const;
    IC const RAW_VISIBLES& raw_objects() const;
    IC const NOT_YET_VISIBLES& not_yet_visible_objects() const;
    const CVisionParameters& current_state() const;
    squad_mask_type mask() const;

public:
#ifdef DEBUG
    void check_visibles() const;
#endif

public:
    void save(NET_Packet& packet) const;
    void load(IReader& packet);
    void on_requested_spawn(IGameObject* object);

private:
    void clear_delayed_objects();
};

#include "visual_memory_manager_inline.h"
