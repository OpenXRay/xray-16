#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "xrCore/xrPool.h"
#include "r_constants.h"

// pool
//.static	poolSS<R_constant,512>			g_constant_allocator;

// R_constant_table::~R_constant_table	()	{	dxRenderDeviceRender::Instance().Resources->_DeleteConstantTable(this);
// }

struct search_entry
{
    cpcstr name;
    const u16 type;
};

R_constant_table::~R_constant_table() { RImplementation.Resources->_DeleteConstantTable(this); }
void R_constant_table::fatal(LPCSTR S) { FATAL(S); }
// predicates
IC bool p_search(const ref_constant& C, cpcstr S) { return xr_strcmp(*C->name, S) < 0; }
IC bool p_sort(const ref_constant& C1, const ref_constant C2) { return xr_strcmp(C1->name, C2->name) < 0; }
ref_constant R_constant_table::get(pcstr S, u16 type /*= u16(-1)*/)
{
    // assumption - sorted by name
    c_table::iterator it;
    if (type == u16(-1))
    {
        it = std::lower_bound(table.begin(), table.end(), S, p_search);
    }
    else
    {
        it = std::find_if(table.begin(), table.end(), [&](const ref_constant& constant)
        {
            return 0 == xr_strcmp(constant->name.c_str(), S) && constant->type == type;
        });
    }

    if (it == table.end() || (0 != xr_strcmp((*it)->name.c_str(), S)))
        return nullptr;
    return *it;
}
ref_constant R_constant_table::get(const shared_str& S, u16 type /*= u16(-1)*/)
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

#ifdef USE_DX9
BOOL R_constant_table::parse(void* _desc, u32 destination)
{
    D3DXSHADER_CONSTANTTABLE* desc = (D3DXSHADER_CONSTANTTABLE*)_desc;
    D3DXSHADER_CONSTANTINFO* it = (D3DXSHADER_CONSTANTINFO*)((u8*)(desc) + desc->ConstantInfo);
    u8* ptr = (u8*)(desc);
    for (u32 dwCount = desc->Constants; dwCount; dwCount--, it++)
    {
        // Name
        LPCSTR name = LPCSTR(ptr + it->Name);

        // Type
        u16 type = RC_float;
        if (D3DXRS_BOOL == it->RegisterSet)
            type = RC_bool;
        if (D3DXRS_INT4 == it->RegisterSet)
            type = RC_int;

        // Rindex,Rcount
        u16 r_index = it->RegisterIndex;
        u16 r_type = u16(-1);

        // TypeInfo + class
        D3DXSHADER_TYPEINFO* T = (D3DXSHADER_TYPEINFO*)(ptr + it->TypeInfo);
        BOOL bSkip = FALSE;
        switch (T->Class)
        {
        case D3DXPC_SCALAR: r_type = RC_1x1; break;
        case D3DXPC_VECTOR: r_type = RC_1x4; break;
        case D3DXPC_MATRIX_ROWS:
        {
            switch (T->Columns)
            {
            case 4:
                switch (T->Rows)
                {
                case 3:
                    switch (it->RegisterCount)
                    {
                    case 2: r_type = RC_2x4; break;
                    case 3: r_type = RC_3x4; break;
                    default:
                        Msg("Invalid matrix dimension:%dx%d in constant %s", it->RegisterCount, T->Columns, name);
                        fatal("MATRIX_ROWS: unsupported number of RegisterCount");
                        break;
                    }
                    break;
                case 4:
                    r_type = RC_4x4;
                    VERIFY(4 == it->RegisterCount);
                    break;
                default: fatal("MATRIX_ROWS: unsupported number of Rows"); break;
                }
                break;
            default: fatal("MATRIX_ROWS: unsupported number of Columns"); break;
            }
        }
        break;
        case D3DXPC_MATRIX_COLUMNS: fatal("Pclass MATRIX_COLUMNS unsupported"); break;
        case D3DXPC_STRUCT: fatal("Pclass D3DXPC_STRUCT unsupported"); break;
        case D3DXPC_OBJECT:
        {
            switch (T->Type)
            {
            case D3DXPT_SAMPLER:
            case D3DXPT_SAMPLER1D:
            case D3DXPT_SAMPLER2D:
            case D3DXPT_SAMPLER3D:
            case D3DXPT_SAMPLERCUBE:
            {
                // ***Register sampler***
                // We have determined all valuable info, search if constant already created
                ref_constant C = get(name);
                if (!C)
                {
                    C = table.emplace_back(xr_new<R_constant>()); //.g_constant_allocator.create();
                    C->name = name;
                    C->destination = RC_dest_sampler;
                    C->type = RC_sampler;
                    R_constant_load& L = C->samp;
                    L.index = u16(r_index + ((destination & 1) ? 0 : D3DVERTEXTEXTURESAMPLER0));
                    L.cls = RC_sampler;
                }
                else
                {
                    R_ASSERT(C->destination == RC_dest_sampler);
                    R_ASSERT(C->type == RC_sampler);
                    R_constant_load& L = C->samp;
                    R_ASSERT(L.index == r_index);
                    R_ASSERT(L.cls == RC_sampler);
                }
            }
            break;
            default: fatal("Pclass D3DXPC_OBJECT - object isn't of 'sampler' type"); break;
            }
        }
            bSkip = TRUE;
            break;
        default: bSkip = TRUE; break;
        }
        if (bSkip)
            continue;

        // We have determined all valuable info, search if constant already created
        ref_constant C = get(name);
        if (!C)
        {
            C = table.emplace_back(xr_new<R_constant>()); //.g_constant_allocator.create();
            C->name = name;
            C->destination = destination;
            C->type = type;
            R_constant_load& L = (destination & 1) ? C->ps : C->vs;
            L.index = r_index;
            L.cls = r_type;
        }
        else
        {
            C->destination |= destination;
            VERIFY(C->type == type);
            R_constant_load& L = (destination & 1) ? C->ps : C->vs;
            L.index = r_index;
            L.cls = r_type;
        }
    }
    std::sort(table.begin(), table.end(), p_sort);
    return TRUE;
}
#endif // USE_DX9

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
#ifdef USE_DX11
            C->hs = src->hs;
            C->ds = src->ds;
            C->cs = src->cs;
#endif
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
        std::sort(table.begin(), table.end(), p_sort);
    }

#if !defined(USE_DX9) && !defined(USE_OGL)
    //	TODO:	DX10:	Implement merge with validity check
    m_CBTable.reserve(m_CBTable.size() + T->m_CBTable.size());
    for (u32 i = 0; i < T->m_CBTable.size(); ++i)
        m_CBTable.push_back(T->m_CBTable[i]);
#endif
}

void R_constant_table::clear()
{
    //.
    for (u32 it = 0; it < table.size(); it++)
        table[it] = 0; //.g_constant_allocator.destroy(table[it]);
    table.clear();
#if !defined(USE_DX9) && !defined(USE_OGL)
    m_CBTable.clear();
#endif
}

BOOL R_constant_table::equal(R_constant_table& C)
{
    if (table.size() != C.table.size())
        return FALSE;
    u32 size = table.size();
    for (u32 it = 0; it < size; it++)
    {
        if (!table[it]->equal(&*C.table[it]))
            return FALSE;
    }

    return TRUE;
}
