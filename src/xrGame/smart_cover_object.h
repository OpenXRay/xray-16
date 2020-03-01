////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_object.h
//	Created 	: 28.08.2007
//  Modified 	: 28.08.2007
//	Author		: Dmitriy Iassenev
//	Description : smart cover object class
////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SMART_COVER_OBJECT_H_INCLUDED
#define SMART_COVER_OBJECT_H_INCLUDED

#include "GameObject.h"
#include "Common/Noncopyable.hpp"

namespace smart_cover
{
class cover;

class object : public CGameObject
{
private:
    typedef CGameObject inherited;

private:
    cover const* m_cover;
    float m_enter_min_enemy_distance;
    float m_exit_min_enemy_distance;

public:
    virtual void Load(LPCSTR section);
    virtual bool feel_touch_on_contact(IGameObject*) { return FALSE; }
    virtual bool use(CGameObject* who_use) { return false; }
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void UpdateCL();
    virtual void shedule_Update(u32 dt);
    virtual bool IsVisibleForZones() { return false; }
    virtual BOOL UsedAI_Locations() { return true; }
    virtual bool can_validate_position_on_spawn() { return false; }
    virtual bool use_parent_ai_locations() const { return false; }
    virtual bool is_ai_obstacle() const { return false; }
    virtual bool register_schedule() const { return false; }
    virtual void Center(Fvector& result) const;
    virtual float Radius() const;
#ifdef DEBUG
    virtual void OnRender();
#endif // DEBUG
    bool inside(Fvector const& position) const;

    IC float const& enter_min_enemy_distance() const;
    IC float const& exit_min_enemy_distance() const;
    IC cover const& get_cover() const;
};

} // namespace smart_cover

typedef smart_cover::object smart_cover__object;

#include "smart_cover_object_inline.h"

#endif // SMART_COVER_OBJECT_H_INCLUDED
