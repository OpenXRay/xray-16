#include "stdafx.h"

#include "Layers/xrRender/r_constants.h"
#include "Layers/xrRenderDX9/dx9shader_utils.h"

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
                    L.index = u16(r_index + ((destination & RC_dest_pixel) ? 0 : D3DVERTEXTEXTURESAMPLER0));
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
            R_constant_load& L = (destination & RC_dest_pixel) ? C->ps : C->vs;
            L.index = r_index;
            L.cls = r_type;
        }
        else
        {
            C->destination |= destination;
            VERIFY(C->type == type);
            R_constant_load& L = (destination & RC_dest_pixel) ? C->ps : C->vs;
            L.index = r_index;
            L.cls = r_type;
        }
    }
    std::sort(table.begin(), table.end(), [](const ref_constant& C1, const ref_constant& C2)
    {
        return xr_strcmp(C1->name, C2->name) < 0;
    });
    return TRUE;
}
