#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "xrCore/xrPool.h"
#include "r_constants.h"

// pool
//.static	poolSS<R_constant,512>			g_constant_allocator;

// R_constant_table::~R_constant_table	()	{	dxRenderDeviceRender::Instance().Resources->_DeleteConstantTable(this);
// }

R_constant_table::~R_constant_table() { RImplementation.Resources->_DeleteConstantTable(this); }
void R_constant_table::fatal(LPCSTR S) { FATAL(S); }
// predicates
IC bool p_search(ref_constant C, LPCSTR S) { return xr_strcmp(*C->name, S) < 0; }
IC bool p_sort(ref_constant C1, ref_constant C2) { return xr_strcmp(C1->name, C2->name) < 0; }
ref_constant R_constant_table::get(LPCSTR S)
{
    // assumption - sorted by name
    c_table::iterator I = std::lower_bound(table.begin(), table.end(), S, p_search);
    if (I == table.end() || (0 != xr_strcmp(*(*I)->name, S)))
        return nullptr;
    else
        return *I;
}
ref_constant R_constant_table::get(shared_str& S)
{
    // linear search, but only ptr-compare
    c_table::iterator I = table.begin();
    c_table::iterator E = table.end();
    for (; I != E; ++I)
    {
        ref_constant C = *I;
        if (C->name.equal(S))
            return C;
    }
    return nullptr;
}

#if !defined(USE_DX10) && !defined(USE_DX11) && !defined(USE_OGL)
BOOL R_constant_table::parse(void* _desc, u32 destination)
{
    D3DXSHADER_CONSTANTTABLE* desc = (D3DXSHADER_CONSTANTTABLE*)_desc;
    D3DXSHADER_CONSTANTINFO* it = (D3DXSHADER_CONSTANTINFO*)(LPBYTE(desc) + desc->ConstantInfo);
    LPBYTE ptr = LPBYTE(desc);
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
                    C = new R_constant(); //.g_constant_allocator.create();
                    C->name = name;
                    C->destination = RC_dest_sampler;
                    C->type = RC_sampler;
                    R_constant_load& L = C->samp;
                    L.index = u16(r_index + ((destination & 1) ? 0 : D3DVERTEXTEXTURESAMPLER0));
                    L.cls = RC_sampler;
                    table.push_back(C);
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
            C = new R_constant(); //.g_constant_allocator.create();
            C->name = name;
            C->destination = destination;
            C->type = type;
            R_constant_load& L = (destination & 1) ? C->ps : C->vs;
            L.index = r_index;
            L.cls = r_type;
            table.push_back(C);
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
#endif //	USE_DX10

/// !!!!!!!!FIX THIS FOR DX11!!!!!!!!!
void R_constant_table::merge(R_constant_table* T)
{
    if (nullptr == T)
        return;

    // Real merge
    for (u32 it = 0; it < T->table.size(); it++)
    {
        ref_constant src = T->table[it];
        ref_constant C = get(*src->name);
        if (!C)
        {
            C = new R_constant(); //.g_constant_allocator.create();
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
            table.push_back(C);
        }
        else
        {
            VERIFY2(!(C->destination & src->destination & RC_dest_sampler),
                "Can't have samplers or textures with the same name for PS, VS and GS.");
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

    // Sort
    std::sort(table.begin(), table.end(), p_sort);

#if defined(USE_DX10) || defined(USE_DX11)
    //	TODO:	DX10:	Implement merge with validity check
    m_CBTable.reserve(m_CBTable.size() + T->m_CBTable.size());
    for (u32 i = 0; i < T->m_CBTable.size(); ++i)
        m_CBTable.push_back(T->m_CBTable[i]);
#endif //	USE_DX10
}

void R_constant_table::clear()
{
    //.
    for (u32 it = 0; it < table.size(); it++)
        table[it] = 0; //.g_constant_allocator.destroy(table[it]);
    table.clear();
#if defined(USE_DX10) || defined(USE_DX11)
    m_CBTable.clear();
#endif //
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
