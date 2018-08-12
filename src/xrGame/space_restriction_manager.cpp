////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_manager.cpp
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "space_restriction.h"
#include "restriction_space.h"
#include "space_restriction_manager.h"
#include "space_restriction_bridge.h"
#include "Common/object_broker.h"

const u32 time_to_delete = 300000;

struct CSpaceRestrictionManager::CClientRestriction
{
    CRestrictionPtr m_restriction;
    shared_str m_base_out_restrictions;
    shared_str m_base_in_restrictions;
};

CSpaceRestrictionManager::CSpaceRestrictionManager() { m_clients = new CLIENT_RESTRICTIONS(); }
CSpaceRestrictionManager::~CSpaceRestrictionManager()
{
    xr_delete(m_clients);
    delete_data(m_space_restrictions);
}

void show_restriction(const shared_str& restrictions)
{
    string256 temp;
    for (int i = 0, n = _GetItemCount(*restrictions); i < n; ++i)
        Msg("     %s", _GetItem(*restrictions, i, temp));
}

typedef intrusive_ptr<CSpaceRestriction, RestrictionSpace::CTimeIntrusiveBase> CRestrictionPtr;
void show_restriction(const CRestrictionPtr& restriction)
{
    Msg("out");
    show_restriction(restriction->out_restrictions());
    Msg("in");
    show_restriction(restriction->in_restrictions());
}

void CSpaceRestrictionManager::clear()
{
    m_clients->clear();
    delete_data(m_space_restrictions);

    CSpaceRestrictionHolder::clear();
}

void CSpaceRestrictionManager::remove_border(ALife::_OBJECT_ID id)
{
    CRestrictionPtr client_restriction = restriction(id);
    if (client_restriction)
        client_restriction->remove_border();
}

shared_str CSpaceRestrictionManager::in_restrictions(ALife::_OBJECT_ID id)
{
    CRestrictionPtr client_restriction = restriction(id);
    if (client_restriction)
        return (client_restriction->in_restrictions());
    return ("");
}

shared_str CSpaceRestrictionManager::out_restrictions(ALife::_OBJECT_ID id)
{
    CRestrictionPtr client_restriction = restriction(id);
    if (client_restriction)
        return (client_restriction->out_restrictions());
    return ("");
}

shared_str CSpaceRestrictionManager::base_in_restrictions(ALife::_OBJECT_ID id)
{
    CLIENT_RESTRICTIONS::iterator I = m_clients->find(id);
    VERIFY(m_clients->end() != I);
    return ((*I).second.m_base_in_restrictions);
}

shared_str CSpaceRestrictionManager::base_out_restrictions(ALife::_OBJECT_ID id)
{
    CLIENT_RESTRICTIONS::iterator I = m_clients->find(id);
    VERIFY(m_clients->end() != I);
    return ((*I).second.m_base_out_restrictions);
}

CSpaceRestrictionManager::CRestrictionPtr CSpaceRestrictionManager::restriction(ALife::_OBJECT_ID id)
{
    CLIENT_RESTRICTIONS::iterator I = m_clients->find(id);
    VERIFY(m_clients->end() != I);
    return ((*I).second.m_restriction);
}

void CSpaceRestrictionManager::collect_garbage()
{
    SPACE_RESTRICTIONS::iterator I = m_space_restrictions.begin(), J;
    SPACE_RESTRICTIONS::iterator E = m_space_restrictions.end();
    for (; I != E;)
    {
        if ((*I).second->released() && (Device.dwTimeGlobal >= (*I).second->m_last_time_dec + time_to_delete))
        {
            J = I;
            ++I;
            xr_delete((*J).second);
            m_space_restrictions.erase(J);
        }
        else
            ++I;
    }
}

void CSpaceRestrictionManager::restrict(ALife::_OBJECT_ID id, shared_str out_restrictors, shared_str in_restrictors)
{
    shared_str merged_out_restrictions = out_restrictors;
    shared_str merged_in_restrictions = in_restrictors;
    shared_str _default_out_restrictions = default_out_restrictions();
    shared_str _default_in_restrictions = default_in_restrictions();

    difference_restrictions(_default_out_restrictions, merged_in_restrictions);
    difference_restrictions(_default_in_restrictions, merged_out_restrictions);

    join_restrictions(merged_out_restrictions, _default_out_restrictions);
    join_restrictions(merged_in_restrictions, _default_in_restrictions);

    CLIENT_RESTRICTIONS::iterator I = m_clients->find(id);
    VERIFY2((m_clients->end() == I) || !(*I).second.m_restriction || !(*I).second.m_restriction->applied(),
        "Restriction cannot be changed since its border is still applied!");
    (*m_clients)[id].m_restriction = restriction(merged_out_restrictions, merged_in_restrictions);
    (*m_clients)[id].m_base_out_restrictions = out_restrictors;
    (*m_clients)[id].m_base_in_restrictions = in_restrictors;

    collect_garbage();
}

void CSpaceRestrictionManager::unrestrict(ALife::_OBJECT_ID id)
{
    CLIENT_RESTRICTIONS::iterator I = m_clients->find(id);
    VERIFY(I != m_clients->end());
    m_clients->erase(I);
    collect_garbage();
}

bool CSpaceRestrictionManager::accessible(ALife::_OBJECT_ID id, const Fsphere& sphere)
{
    CRestrictionPtr client_restriction = restriction(id);
    if (client_restriction)
        return (client_restriction->accessible(sphere));
    return (true);
}

bool CSpaceRestrictionManager::accessible(ALife::_OBJECT_ID id, u32 level_vertex_id, float radius)
{
    CRestrictionPtr client_restriction = restriction(id);
    if (client_restriction)
        return (client_restriction->accessible(level_vertex_id, radius));
    return (true);
}

CSpaceRestrictionManager::CRestrictionPtr CSpaceRestrictionManager::restriction(
    shared_str out_restrictors, shared_str in_restrictors)
{
    string4096 m_temp;
    if (!xr_strlen(out_restrictors) && !xr_strlen(in_restrictors))
        return (0);

    out_restrictors = normalize_string(out_restrictors);
    in_restrictors = normalize_string(in_restrictors);

    strconcat(sizeof(m_temp), m_temp, *out_restrictors, "\x01", *in_restrictors);
    shared_str space_restrictions = m_temp;

    SPACE_RESTRICTIONS::const_iterator I = m_space_restrictions.find(space_restrictions);
    if (I != m_space_restrictions.end())
        return ((*I).second);

    CSpaceRestriction* client_restriction = new CSpaceRestriction(this, out_restrictors, in_restrictors);
    m_space_restrictions.insert(std::make_pair(space_restrictions, client_restriction));
    return (client_restriction);
}

u32 CSpaceRestrictionManager::accessible_nearest(ALife::_OBJECT_ID id, const Fvector& position, Fvector& result)
{
    CRestrictionPtr client_restriction = restriction(id);
    VERIFY(client_restriction);
    return (client_restriction->accessible_nearest(position, result));
}

bool CSpaceRestrictionManager::restriction_presented(shared_str restrictions, shared_str restriction) const
{
    string4096 m_temp;
    for (u32 i = 0, n = _GetItemCount(*restrictions); i < n; ++i)
        if (!xr_strcmp(restriction, _GetItem(*restrictions, i, m_temp)))
            return (true);
    return (false);
}

void CSpaceRestrictionManager::join_restrictions(shared_str& restrictions, shared_str update)
{
    string4096 m_temp1;
    string4096 m_temp2;
    xr_strcpy(m_temp2, *restrictions);
    for (u32 i = 0, n = _GetItemCount(*update), count = xr_strlen(m_temp2); i < n; ++i)
        if (!restriction_presented(m_temp2, _GetItem(*update, i, m_temp1)))
        {
            if (count)
                xr_strcat(m_temp2, ",");
            xr_strcat(m_temp2, m_temp1);
            ++count;
        }
    restrictions = shared_str(m_temp2);
}

void CSpaceRestrictionManager::difference_restrictions(shared_str& restrictions, shared_str update)
{
    string4096 m_temp1;
    string4096 m_temp2;
    xr_strcpy(m_temp2, "");
    for (u32 i = 0, n = _GetItemCount(*restrictions), count = 0; i < n; ++i)
        if (!restriction_presented(update, _GetItem(*restrictions, i, m_temp1)))
        {
            if (count)
                xr_strcat(m_temp2, ",");
            xr_strcat(m_temp2, m_temp1);
            ++count;
        }
    restrictions = shared_str(m_temp2);
}

void CSpaceRestrictionManager::add_restrictions(
    ALife::_OBJECT_ID id, shared_str add_out_restrictions, shared_str add_in_restrictions)
{
    CRestrictionPtr _client_restriction = restriction(id);
    if (!_client_restriction)
    {
        restrict(id, add_out_restrictions, add_in_restrictions);
        return;
    }

    VERIFY(!_client_restriction->applied());

    CClientRestriction& client_restriction = (*m_clients)[id];

    shared_str new_out_restrictions = client_restriction.m_base_out_restrictions;
    shared_str new_in_restrictions = client_restriction.m_base_in_restrictions;

    join_restrictions(new_out_restrictions, add_out_restrictions);
    join_restrictions(new_in_restrictions, add_in_restrictions);

    restrict(id, new_out_restrictions, new_in_restrictions);
}

void CSpaceRestrictionManager::remove_restrictions(
    ALife::_OBJECT_ID id, shared_str remove_out_restrictions, shared_str remove_in_restrictions)
{
    CRestrictionPtr _client_restriction = restriction(id);
    if (!_client_restriction)
        return;

    VERIFY(!_client_restriction->applied());

    CClientRestriction& client_restriction = (*m_clients)[id];

    shared_str new_out_restrictions = client_restriction.m_base_out_restrictions;
    shared_str new_in_restrictions = client_restriction.m_base_in_restrictions;

    difference_restrictions(new_out_restrictions, remove_out_restrictions);
    difference_restrictions(new_in_restrictions, remove_in_restrictions);

    restrict(id, new_out_restrictions, new_in_restrictions);
}

void CSpaceRestrictionManager::change_restrictions(ALife::_OBJECT_ID id, shared_str add_out_restrictions,
    shared_str add_in_restrictions, shared_str remove_out_restrictions, shared_str remove_in_restrictions)
{
    CRestrictionPtr _client_restriction = restriction(id);
    if (!_client_restriction)
    {
        restrict(id, add_out_restrictions, add_in_restrictions);
        return;
    }

    VERIFY(!_client_restriction->applied());

    CClientRestriction& client_restriction = (*m_clients)[id];

    shared_str new_out_restrictions = client_restriction.m_base_out_restrictions;
    shared_str new_in_restrictions = client_restriction.m_base_in_restrictions;

    difference_restrictions(new_out_restrictions, remove_out_restrictions);
    difference_restrictions(new_in_restrictions, remove_in_restrictions);

    join_restrictions(new_out_restrictions, add_out_restrictions);
    join_restrictions(new_in_restrictions, add_in_restrictions);

    restrict(id, new_out_restrictions, new_in_restrictions);
}

void CSpaceRestrictionManager::on_default_restrictions_changed()
{
    CLIENT_RESTRICTIONS::const_iterator I = m_clients->begin();
    CLIENT_RESTRICTIONS::const_iterator E = m_clients->end();
    for (; I != E; ++I)
        restrict((*I).first, (*I).second.m_base_out_restrictions, (*I).second.m_base_in_restrictions);
}
