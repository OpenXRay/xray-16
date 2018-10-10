#include "StdAfx.h"

#include "Restrictions.h"
#ifdef DEBUG
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"
#endif //#ifdef DEBUG
#include "string_table.h"
CRestrictions g_mp_restrictions;

shared_str g_ranks[_RANK_COUNT];

u32 get_rank(const shared_str& section)
{
    int res = -1;
    if (g_ranks[0].size() == 0)
    { // load
        string32 buff;
        for (int i = 0; i < _RANK_COUNT; i++)
        {
            xr_sprintf(buff, "rank_%d", i);
            g_ranks[i] = pSettings->r_string(buff, "available_items");
        }
    }
    for (u32 i = 0; i < _RANK_COUNT; i++)
    {
        if (strstr(g_ranks[i].c_str(), section.c_str()))
        {
            res = i;
            break;
        }
    }

    if (res == -1)
    {
        Msg("Setting rank to 0. Cannot find rank for: [%s]", section.c_str());
        // Xottab_DUTY: I'm not sure if it's save to leave it -1
        res = 0;
    }
    //R_ASSERT3(res != -1, "cannot find rank for", section.c_str());
    return res;
}

CRestrictions::CRestrictions()
{
    m_rank = 0;
    m_bInited = false;
}

CRestrictions::~CRestrictions() {}
void CRestrictions::InitGroups()
{
    if (m_bInited)
        return;
    m_bInited = true;

    // create groups
    u32 c = pSettings->line_count("mp_item_groups");
    LPCSTR line, name;

    for (u32 i = 0; i < c; ++i)
    {
        pSettings->r_line("mp_item_groups", i, &name, &line);
        AddGroup(name, line);
    }

    // try to find restrictions in every rank

    AddRestriction4rank(_RANK_COUNT, pSettings->r_string("rank_base", "amount_restriction"));

    string32 rank;
    for (u32 i = 0; i < _RANK_COUNT; ++i)
    {
        xr_sprintf(rank, "rank_%d", i);

        AddRestriction4rank(i, pSettings->r_string(rank, "amount_restriction"));
        m_names[i] = StringTable().translate(pSettings->r_string(rank, "rank_name"));
    }

#ifndef MASTER_GOLD
    Dump();
#endif // #ifndef MASTER_GOLD

#ifdef DEBUG
    CMD4(CCC_Integer, "rank_for_buymenu", (int*)&m_rank, 0, 4);
#endif
}

void CRestrictions::AddRestriction4rank(u32 rank, const shared_str& lst)
{ // private
    VERIFY(m_bInited);

    rank_rest_vec& rest = m_restrictions[rank];

    if (rank != _RANK_COUNT)
    {
        u32 src_idx = (rank == 0) ? _RANK_COUNT : (rank - 1);
        rest = m_restrictions[src_idx];
    }

    string256 singleItem;
    u32 count = _GetItemCount(lst.c_str());
    for (u32 j = 0; j < count; ++j)
    {
        _GetItem(lst.c_str(), j, singleItem);
        RESTR r = GetRestr(singleItem);
        restr_item* ritem = find_restr_item_internal(rank, r.name);
        VERIFY2((ritem || rank == _RANK_COUNT), singleItem);
        if (!ritem)
            rest.push_back(std::make_pair(r.name, r.n));
        else
            ritem->second = r.n;
    }
}

bool CRestrictions::IsAvailable(const shared_str& itm)
{
    u32 _r = get_rank(itm);
    return (_r <= m_rank);
}

void CRestrictions::AddGroup(LPCSTR group, LPCSTR lst)
{ // private
    VERIFY(m_bInited);

    VERIFY(m_goups.find(group) == m_goups.end());

    group_items& _new = m_goups[group];
    string256 singleItem;
    u32 count = _GetItemCount(lst);
    for (u32 j = 0; j < count; ++j)
    {
        _GetItem(lst, j, singleItem);
#ifdef DEBUG
        const shared_str& _exist = GetItemGroup(singleItem);
        VERIFY3(_exist.size() == 0, "item has duplicate record in groups", singleItem);
#endif
        _new.push_back(singleItem);
    }
}

bool CRestrictions::IsGroupExist(const shared_str& group) const { return (m_goups.end() != m_goups.find(group)); }
RESTR CRestrictions::GetRestr(const shared_str& item)
{ // private function
    VERIFY(m_bInited);
    RESTR ret;
    string512 _name;
    int _cnt = 0;
    ptrdiff_t n = strchr(item.c_str(), ':') - item.c_str();
    if (n > 0)
    {
        strncpy_s(_name, item.c_str(), n);
        _name[n] = 0;
        _cnt = sscanf(item.c_str() + n + 1, "%d", &ret.n);
    }
    R_ASSERT3(n > 0 && _cnt == 1, "invalid record format <name_sect:rank>", item.c_str());
    ret.name = _name;

    return ret;
}

shared_str CRestrictions::GetItemGroup(const shared_str& item) const
{ // private function
    VERIFY(m_bInited);
    Groups::const_iterator it;
    group_items::const_iterator IT;

    for (it = m_goups.begin(); it != m_goups.end(); it++)
        for (IT = (*it).second.begin(); IT != (*it).second.end(); IT++)
            if ((*IT) == item)
                return (*it).first;

    return NULL;
}

u32 CRestrictions::GetItemCount(const shared_str& item_sect) const
{
    VERIFY(m_bInited);

    const shared_str& group_name = GetItemGroup(item_sect);
    VERIFY3(group_name.size(), "item has no group", item_sect.c_str());
    const restr_item* res = find_restr_item(m_rank, group_name);

    VERIFY4(res, "group has no restrictions for rank", group_name.c_str(), GetRankName(m_rank).c_str());

    return res->second;
}

u32 CRestrictions::GetGroupCount(const shared_str& group_name) const
{
    VERIFY(m_bInited);
    const restr_item* res = find_restr_item(m_rank, group_name);

    VERIFY3(res, "group has no restrictions for rank", GetRankName(m_rank).c_str());

    return res->second;
}

CRestrictions::restr_item* CRestrictions::find_restr_item_internal(const u32& rank, const shared_str& group_name)
{
    VERIFY(m_bInited);
    rank_rest_vec::iterator it = m_restrictions[rank].begin();
    rank_rest_vec::iterator it_e = m_restrictions[rank].end();

    for (; it != it_e; ++it)
    {
        if (it->first == group_name)
            return &(*it);
    }
    return NULL;
}

const CRestrictions::restr_item* CRestrictions::find_restr_item(const u32& rank, const shared_str& group_name) const
{
    VERIFY(m_bInited);
    rank_rest_vec::const_iterator it = m_restrictions[rank].begin();
    rank_rest_vec::const_iterator it_e = m_restrictions[rank].end();

    for (; it != it_e; ++it)
    {
        if (it->first == group_name)
            return &(*it);
    }
    VERIFY(it != it_e);
    return NULL;
}

void CRestrictions::Dump() const
{
#ifndef MASTER_GOLD
    Msg("------------item groups ---count=[%d]-------------------", m_goups.size());
    Groups::const_iterator it = m_goups.begin();
    Groups::const_iterator it_e = m_goups.end();
    for (; it != it_e; ++it)
    {
        Msg("group [%s]", it->first.c_str());
        group_items::const_iterator it2 = it->second.begin();
        group_items::const_iterator it2_e = it->second.end();
        for (; it2 != it2_e; ++it2)
            Msg("	[%s]", (*it2).c_str());
    }
    Msg("------------rank restrictions------------");
    for (u32 i = 0; i < _RANK_COUNT + 1; ++i)
    {
        const rank_rest_vec& v = m_restrictions[i];
        rank_rest_vec::const_iterator it = v.begin();
        rank_rest_vec::const_iterator it_e = v.end();
        if (i < _RANK_COUNT)
            Msg("---	for rank %d  ---count=[%d]", i, v.size());
        else
            Msg("---	base restrictions ---count=[%d]", v.size());

        for (; it != it_e; ++it)
        {
            Msg("	[%s]:[%d]", (*it).first.c_str(), (*it).second);
        }
        Msg("-----------------------------------------");
    }
#endif // #ifndef MASTER_GOLD
}
