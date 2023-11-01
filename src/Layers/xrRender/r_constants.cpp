#include "stdafx.h"

#include "r_constants.h"
#include "ResourceManager.h"

//.#include "xrCore/xrPool.h"

// pool
//.static	poolSS<R_constant,512>			g_constant_allocator;

R_constant_table::~R_constant_table()
{
    RImplementation.Resources->_DeleteConstantTable(this);
}

void R_constant_table::fatal(LPCSTR S)
{
    FATAL(S);
}

ref_constant R_constant_table::get(pcstr S, u16 type /*= u16(-1)*/) const
{
    // assumption - sorted by name
    c_table::const_iterator it;
    if (type == u16(-1))
    {
        it = std::lower_bound(table.cbegin(), table.cend(), S, [](const ref_constant& C, cpcstr S)
        {
            return xr_strcmp(*C->name, S) < 0;
        });
    }
    else
    {
        it = std::find_if(table.cbegin(), table.cend(), [&](const ref_constant& constant)
        {
            return 0 == xr_strcmp(constant->name.c_str(), S) && constant->type == type;
        });
    }

    if (it == table.cend() || (0 != xr_strcmp((*it)->name.c_str(), S)))
        return nullptr;
    return *it;
}

ref_constant R_constant_table::get(const shared_str& S, u16 type /*= u16(-1)*/) const
{
    // linear search, but only ptr-compare
    if (type == u16(-1))
    {
        for (const ref_constant& C : table)
        {
            if (C->name.equal(S))
                return C;
        }
    }
    else
    {
        for (const ref_constant& C : table)
        {
            if (C->name.equal(S) && C->type == type)
                return C;
        }
    }

    return nullptr;
}

/// !!!!!!!!FIX THIS FOR DX11!!!!!!!!!
void R_constant_table::merge(R_constant_table* T)
{
    if (nullptr == T)
        return;

    if (T->dx9compatibility)
        dx9compatibility = true;

    // Real merge
    xr_vector<ref_constant> table_tmp;
    table_tmp.reserve(table.size());
    for (u32 it = 0; it < T->table.size(); it++)
    {
        ref_constant src = T->table[it];
        ref_constant C = get(*src->name, dx9compatibility ? src->type : u16(-1));
        if (!C || (dx9compatibility && C->type != src->type))
        {
            C = xr_new<R_constant>(); //.g_constant_allocator.create();
            C->name = src->name;
            C->destination = src->destination;
            C->type = src->type;
            C->ps = src->ps;
            C->vs = src->vs;
#ifndef USE_DX9
            C->gs = src->gs;
#   if defined(USE_DX11)
            C->hs = src->hs;
            C->ds = src->ds;
            C->cs = src->cs;
#   endif
#endif
#ifdef USE_OGL
            C->pp = src->pp;
#endif
            C->samp = src->samp;
            C->handler = src->handler;
            table_tmp.push_back(C);
        }
        else
        {
            C->destination |= src->destination;
            VERIFY(C->type == src->type);
            R_constant_load& sL = src->get_load(src->destination);
            R_constant_load& dL = C->get_load(src->destination);
            dL.index = sL.index;
            dL.cls = sL.cls;
#ifdef USE_OGL
            dL.location = sL.location;
            dL.program = sL.program;
#endif // USE_OGL
        }
    }

    if (!table_tmp.empty())
    {
        // Append
        std::move(table_tmp.begin(), table_tmp.end(), std::back_inserter(table));

        // Sort
        std::sort(table.begin(), table.end(), [](const ref_constant& C1, const ref_constant& C2)
        {
            return xr_strcmp(C1->name, C2->name) < 0;
        });
    }

#if defined(USE_DX11)
    //	TODO:	DX11:	Implement merge with validity check
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        m_CBTable[id].reserve(m_CBTable[id].size() + T->m_CBTable[id].size());
        for (u32 i = 0; i < T->m_CBTable[id].size(); ++i)
            m_CBTable[id].push_back((T->m_CBTable[id])[i]);
    }
#endif
}

void R_constant_table::clear()
{
    //.
    for (u32 it = 0; it < table.size(); it++)
        table[it] = 0; //.g_constant_allocator.destroy(table[it]);
    table.clear();
#if defined(USE_DX11)
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        m_CBTable[id].clear();
    }
#endif
}

BOOL R_constant_table::equal(R_constant_table& C)
{
    if (table.size() != C.table.size())
        return FALSE;
    const size_t size = table.size();
    for (size_t it = 0; it < size; it++)
    {
        if (!table[it]->equal(*C.table[it]))
            return FALSE;
    }

    return TRUE;
}
