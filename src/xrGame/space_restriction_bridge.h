////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_bridge.h
//	Created 	: 27.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction bridge
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "restriction_space.h"
#include "space_restriction_base.h"

class CSpaceRestrictionBridge : public RestrictionSpace::CTimeIntrusiveBase
{
protected:
    CSpaceRestrictionBase* m_object;

public:
    IC CSpaceRestrictionBase& object() const;

public:
    IC CSpaceRestrictionBridge(CSpaceRestrictionBase* object);
    virtual ~CSpaceRestrictionBridge();
    void change_implementation(CSpaceRestrictionBase* object);
    const xr_vector<u32>& border() const;
    bool initialized() const;
    void initialize();
    bool inside(const Fsphere& sphere);
    bool inside(u32 level_vertex_id, bool partially_inside);
    bool inside(u32 level_vertex_id, bool partially_inside, float radius);
    shared_str name() const;
    u32 accessible_nearest(const Fvector& position, Fvector& result, bool out_restriction);
    bool shape() const;
    bool default_restrictor() const;
    bool on_border(const Fvector& position) const;
    bool out_of_border(const Fvector& position);
    Fsphere sphere() const;

    template <typename T>
    IC u32 accessible_nearest(T& restriction, const Fvector& position, Fvector& result, bool out_restriction);
    template <typename T>
    IC const xr_vector<u32>& accessible_neighbour_border(T& restriction, bool out_restriction);
};

#include "space_restriction_bridge_inline.h"
