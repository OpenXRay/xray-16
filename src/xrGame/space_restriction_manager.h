////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_manager.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "space_restriction_holder.h"
#include "alife_space.h"
#include "space_restriction.h"

namespace RestrictionSpace
{
struct CTimeIntrusiveBase;
};

template <typename _1, typename _2>
class intrusive_ptr;

class CSpaceRestrictionManager : public CSpaceRestrictionHolder
{
    friend class LevelGraphDebugRender;
    struct CClientRestriction;

protected:
    typedef intrusive_ptr<CSpaceRestriction, RestrictionSpace::CTimeIntrusiveBase> CRestrictionPtr;
    typedef xr_map<shared_str, CSpaceRestriction*> SPACE_RESTRICTIONS;
    typedef xr_map<ALife::_OBJECT_ID, CClientRestriction> CLIENT_RESTRICTIONS;

private:
protected:
    SPACE_RESTRICTIONS m_space_restrictions;
    CLIENT_RESTRICTIONS* m_clients;

public:
    using CSpaceRestrictionHolder::restriction;

protected:
    void join_restrictions(shared_str& restrictions, shared_str update);
    void difference_restrictions(shared_str& restrictions, shared_str update);
    CRestrictionPtr restriction(ALife::_OBJECT_ID id);
    CRestrictionPtr restriction(shared_str out_restrictors, shared_str in_restrictors);
    void collect_garbage();
    virtual void on_default_restrictions_changed();

public:
    CSpaceRestrictionManager();
    virtual ~CSpaceRestrictionManager();
    void restrict(ALife::_OBJECT_ID id, shared_str out_restrictors, shared_str in_restrictors);
    void unrestrict(ALife::_OBJECT_ID id);
    void add_restrictions(ALife::_OBJECT_ID id, shared_str out_restrictions, shared_str in_restrictions);
    void remove_restrictions(ALife::_OBJECT_ID id, shared_str out_restrictions, shared_str in_restrictions);
    void change_restrictions(ALife::_OBJECT_ID id, shared_str add_out_restrictions, shared_str add_in_restrictions,
        shared_str remove_out_restrictions, shared_str remove_in_restrictions);
    void clear();

    template <typename T1, typename T2>
    IC void add_border(ALife::_OBJECT_ID id, T1 p1, T2 p2);
    void remove_border(ALife::_OBJECT_ID id);

    shared_str in_restrictions(ALife::_OBJECT_ID id);
    shared_str out_restrictions(ALife::_OBJECT_ID id);

    shared_str base_in_restrictions(ALife::_OBJECT_ID id);
    shared_str base_out_restrictions(ALife::_OBJECT_ID id);

    bool accessible(ALife::_OBJECT_ID id, const Fsphere& sphere);
    bool accessible(ALife::_OBJECT_ID id, u32 level_vertex_id, float radius);
    u32 accessible_nearest(ALife::_OBJECT_ID id, const Fvector& position, Fvector& result);

    bool restriction_presented(shared_str restrictions, shared_str restriction) const;

#ifdef DEBUG
    IC const SPACE_RESTRICTIONS& restrictions() const;
#endif
};

#include "space_restriction_manager_inline.h"
