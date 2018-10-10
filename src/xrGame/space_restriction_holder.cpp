////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_holder.cpp
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction holder
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "space_restriction_holder.h"
#include "Common/object_broker.h"
#include "space_restrictor.h"
#include "space_restriction_bridge.h"
#include "space_restriction_shape.h"
#include "space_restriction_composition.h"
#include "restriction_space.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

const u32 time_to_delete = 300000;

CSpaceRestrictionHolder::~CSpaceRestrictionHolder() { clear(); }
void CSpaceRestrictionHolder::clear()
{
    delete_data(m_restrictions);
    m_default_out_restrictions = "";
    m_default_in_restrictions = "";
}

shared_str CSpaceRestrictionHolder::normalize_string(shared_str space_restrictors)
{
    u32 n = xr_strlen(space_restrictors);
    if (!n)
        return ("");

    // 1. parse the string, copying to temp buffer with leading zeroes, storing pointers in vector
    LPSTR* strings = (LPSTR*)_alloca(MAX_RESTRICTION_PER_TYPE_COUNT * sizeof(LPSTR));
    LPSTR* string_current = strings;

    LPSTR temp_string = (LPSTR)_alloca((n + 1) * sizeof(char));
    LPCSTR I = *space_restrictors;
    LPSTR i = temp_string, j = i;
    for (; *I; ++I, ++i)
    {
        if (*I != ',')
        {
            *i = *I;
            continue;
        }

        *i = 0;
        VERIFY(u32(string_current - strings) < MAX_RESTRICTION_PER_TYPE_COUNT);
        *string_current = j;
        ++string_current;
        j = i + 1;
    }
    if (string_current == strings)
        return (space_restrictors);

    *i = 0;
    VERIFY(u32(string_current - strings) < MAX_RESTRICTION_PER_TYPE_COUNT);
    *string_current = j;
    ++string_current;

    // 2. sort the vector (svector???)
    std::sort(strings, string_current, pred_str());

    // 3. copy back to another temp string, based on sorted vector
    LPSTR result_string = (LPSTR)_alloca((n + 1) * sizeof(char));
    LPSTR pointer = result_string;
    {
        LPSTR* I = strings;
        LPSTR* E = string_current;
        for (; I != E; ++I)
        {
            for (LPSTR i = *I; *i; ++i, ++pointer)
                *pointer = *i;

            *pointer = ',';
            ++pointer;
        }
    }
    *(pointer - 1) = 0;

    // 4. finally, dock shared_str
    return (result_string);
}

SpaceRestrictionHolder::CBaseRestrictionPtr CSpaceRestrictionHolder::restriction(shared_str space_restrictors)
{
    if (!xr_strlen(space_restrictors))
        return (0);

    space_restrictors = normalize_string(space_restrictors);

    RESTRICTIONS::const_iterator I = m_restrictions.find(space_restrictors);
    if (I != m_restrictions.end())
        return ((*I).second);

    collect_garbage();

    CSpaceRestrictionBase* composition = new CSpaceRestrictionComposition(this, space_restrictors);
    CSpaceRestrictionBridge* bridge = new CSpaceRestrictionBridge(composition);
    m_restrictions.insert(std::make_pair(space_restrictors, bridge));
    return (bridge);
}

void CSpaceRestrictionHolder::register_restrictor(
    CSpaceRestrictor* space_restrictor, const RestrictionSpace::ERestrictorTypes& restrictor_type)
{
    string4096 m_temp_string;
    shared_str space_restrictors = space_restrictor->cName();
    if (restrictor_type != RestrictionSpace::eDefaultRestrictorTypeNone)
    {
        shared_str *temp = 0, temp1;
        if (restrictor_type == RestrictionSpace::eDefaultRestrictorTypeOut)
            temp = &m_default_out_restrictions;
        else if (restrictor_type == RestrictionSpace::eDefaultRestrictorTypeIn)
            temp = &m_default_in_restrictions;
        else
            NODEFAULT;
        temp1 = *temp;

        if (xr_strlen(*temp) && xr_strlen(space_restrictors))
            strconcat(sizeof(m_temp_string), m_temp_string, **temp, ",", *space_restrictors);
        else
            strconcat(sizeof(m_temp_string), m_temp_string, **temp, *space_restrictors);

        *temp = normalize_string(m_temp_string);

        if (xr_strcmp(*temp, temp1))
            on_default_restrictions_changed();
    }

    CSpaceRestrictionShape* shape =
        new CSpaceRestrictionShape(space_restrictor, restrictor_type != RestrictionSpace::eDefaultRestrictorTypeNone);
    RESTRICTIONS::iterator I = m_restrictions.find(space_restrictors);
    if (I == m_restrictions.end())
    {
        CSpaceRestrictionBridge* bridge = new CSpaceRestrictionBridge(shape);
        m_restrictions.insert(std::make_pair(space_restrictors, bridge));
        return;
    }

    (*I).second->change_implementation(shape);
}

bool try_remove_string(shared_str& search_string, const shared_str& string_to_search)
{
    bool found = false;
    string256 temp;
    string4096 temp1;
    *temp1 = 0;
    for (int i = 0, j = 0, n = _GetItemCount(*search_string); i < n; ++i, ++j)
    {
        if (xr_strcmp(string_to_search, _GetItem(*search_string, i, temp)))
        {
            if (j)
                xr_strcat(temp1, ",");
            xr_strcat(temp1, temp);
            continue;
        }

        found = true;
        --j;
    }

    if (!found)
        return (false);

    search_string = temp1;
    return (true);
}

void CSpaceRestrictionHolder::unregister_restrictor(CSpaceRestrictor* space_restrictor)
{
    shared_str restrictor_id = space_restrictor->cName();
    RESTRICTIONS::iterator I = m_restrictions.find(restrictor_id);
    VERIFY(I != m_restrictions.end());

    CSpaceRestrictionBridge* bridge = (*I).second;
    m_restrictions.erase(I);

    if (try_remove_string(m_default_out_restrictions, restrictor_id))
        on_default_restrictions_changed();
    else
    {
        if (try_remove_string(m_default_in_restrictions, restrictor_id))
            on_default_restrictions_changed();
    }

    CSpaceRestrictionBase* composition = new CSpaceRestrictionComposition(this, restrictor_id);
    bridge->change_implementation(composition);
    m_restrictions.insert(std::make_pair(restrictor_id, bridge));

    collect_garbage();
}

IC void CSpaceRestrictionHolder::collect_garbage()
{
    RESTRICTIONS::iterator I = m_restrictions.begin(), J;
    RESTRICTIONS::iterator E = m_restrictions.end();
    for (; I != E;)
    {
        if (!(*I).second->shape() && (*I).second->released() &&
            (Device.dwTimeGlobal >= (*I).second->m_last_time_dec + time_to_delete))
        {
            J = I;
            ++I;
            xr_delete((*J).second);
            m_restrictions.erase(J);
        }
        else
            ++I;
    }
}
