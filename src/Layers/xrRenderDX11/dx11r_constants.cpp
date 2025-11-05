#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/ResourceManager.h"
#include "xrCore/xrPool.h"
#include "Layers/xrRender/r_constants.h"
#include "Layers/xrRenderDX11/dx11ConstantBuffer.h"

namespace xray::render::RENDER_NAMESPACE
{
BOOL R_constant_table::parseConstants(ID3DShaderReflectionConstantBuffer* pTable, u32 destination)
{
    // VERIFY(_desc);
    // ID3D10ShaderReflectionConstantBuffer *pTable = (ID3D10ShaderReflectionConstantBuffer *)_desc;
    VERIFY(pTable);
    D3D_SHADER_BUFFER_DESC TableDesc;
    CHK_DX(pTable->GetDesc(&TableDesc));

    // D3DXSHADER_CONSTANTTABLE* desc   = (D3DXSHADER_CONSTANTTABLE*) _desc;
    // D3DXSHADER_CONSTANTINFO* it      = (D3DXSHADER_CONSTANTINFO*) (LPBYTE(desc)+desc->ConstantInfo);
    // LPBYTE                    ptr    = LPBYTE(desc);
    // for (u32 dwCount = desc->Constants; dwCount; dwCount--,it++)
    for (u32 i = 0; i < TableDesc.Variables; ++i)
    {
        ID3DShaderReflectionVariable* pVar;
        D3D_SHADER_VARIABLE_DESC VarDesc;
        ID3DShaderReflectionType* pType;
        D3D_SHADER_TYPE_DESC TypeDesc;

        pVar = pTable->GetVariableByIndex(i);
        VERIFY(pVar);
        pVar->GetDesc(&VarDesc);
        pType = pVar->GetType();
        VERIFY(pType);
        pType->GetDesc(&TypeDesc);

        // Name
        // LPCSTR   name        =   LPCSTR(ptr+it->Name);
        LPCSTR name = VarDesc.Name;

        // Check Class first - skip structs and objects before type validation
        // TypeInfo + class
        // D3DXSHADER_TYPEINFO* T   = (D3DXSHADER_TYPEINFO*)(ptr+it->TypeInfo);
        BOOL bSkip = FALSE;

        // Skip structs and objects early
        if (TypeDesc.Class == D3D_SVC_STRUCT || TypeDesc.Class == D3D_SVC_OBJECT)
        {
            bSkip = TRUE;
        }

        // Type
        // u16      type        =   RC_float;
        u16 type = u16(-1);

        if (!bSkip)
        {
            switch (TypeDesc.Type)
            {
            case D3D_SVT_FLOAT: type = RC_float; break;
            case D3D_SVT_BOOL: type = RC_bool; break;
            case D3D_SVT_INT: type = RC_int; break;
            case D3D_SVT_UINT: type = RC_uint; break;
            case D3D_SVT_VOID: bSkip = TRUE; break; // Skip void types (used for struct metadata)
            default:
            {
                string512 msg;
                xr_sprintf(msg, "R_constant_table::parse: unexpected shader variable type. Name='%s', Type=%d, Class=%d",
                           name, TypeDesc.Type, TypeDesc.Class);
                fatal(msg);
            }
            }
        }

        // Rindex,Rcount
        // u16      r_index     =   it->RegisterIndex;
        //  Used as byte offset in constant buffer
        VERIFY(VarDesc.StartOffset < 0x10000);
        u16 r_index = u16(VarDesc.StartOffset);
        u16 r_type = u16(-1);

        // switch (T->Class)
        if (!bSkip)
        switch (TypeDesc.Class)
        {
        case D3D_SVC_SCALAR: r_type = RC_1x1; break;
        case D3D_SVC_VECTOR:
        {
            switch (TypeDesc.Columns)
            {
            case 4: r_type = RC_1x4; break;
            case 3: r_type = RC_1x3; break;
            case 2: r_type = RC_1x2; break;
            default: fatal("Vector: 1 components is scalar - there is special case for this!!!!!"); break;
            }
        }
        break;
        case D3D_SVC_MATRIX_ROWS:
        {
            switch (TypeDesc.Columns)
            {
            case 4:
                switch (TypeDesc.Rows)
                {
                case 2: r_type = RC_2x4; break;
                case 3:
                    r_type = RC_3x4;
                    break;
                /*
                switch (it->RegisterCount)
                {
                case 2: r_type  =   RC_2x4; break;
                case 3: r_type  =   RC_3x4; break;
                default:
                fatal       ("MATRIX_ROWS: unsupported number of RegisterCount");
                break;
                }
                break;
                */
                case 4:
                    r_type = RC_4x4;
                    // VERIFY(4 == it->RegisterCount);
                    break;
                default: fatal("MATRIX_ROWS: unsupported number of Rows"); break;
                }
                break;
            default: fatal("MATRIX_ROWS: unsupported number of Columns"); break;
            }
        }
        break;
        case D3D_SVC_MATRIX_COLUMNS: fatal("Pclass MATRIX_COLUMNS unsupported"); break;
        // D3D_SVC_STRUCT and D3D_SVC_OBJECT are handled early before type checking
        // Unknown classes are skipped
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
            R_constant_load& L = C->get_load(destination);
            L.index = r_index;
            L.cls = r_type;
        }
        else
        {
            C->destination |= destination;
            VERIFY(C->type == type);
            R_constant_load& L = C->get_load(destination);
            L.index = r_index;
            L.cls = r_type;
        }
    }
    return TRUE;
}

BOOL R_constant_table::parseResources(ID3DShaderReflection* pReflection, int ResNum, u32 destination)
{
    for (int i = 0; i < ResNum; ++i)
    {
        D3D_SHADER_INPUT_BIND_DESC ResDesc;
        pReflection->GetResourceBindingDesc(i, &ResDesc);

        u16 type = 0;

        switch (ResDesc.Type)
        {
        case D3D_SIT_TEXTURE: type = RC_dx11texture; break;
        case D3D_SIT_SAMPLER: type = RC_sampler; break;
        case D3D_SIT_UAV_RWTYPED: type = RC_dx11UAV; break;
        case D3D_SIT_STRUCTURED: type = RC_dx11texture; break; // StructuredBuffer (read-only)
        case D3D_SIT_UAV_RWSTRUCTURED: type = RC_dx11UAV; break; // RWStructuredBuffer
        case D3D_SIT_BYTEADDRESS: type = RC_dx11texture; break; // ByteAddressBuffer
        case D3D_SIT_UAV_RWBYTEADDRESS: type = RC_dx11UAV; break; // RWByteAddressBuffer
        case D3D_SIT_UAV_APPEND_STRUCTURED: type = RC_dx11UAV; break; // AppendStructuredBuffer
        case D3D_SIT_UAV_CONSUME_STRUCTURED: type = RC_dx11UAV; break; // ConsumeStructuredBuffer
        default: continue;
        }

        VERIFY(ResDesc.BindCount == 1);

        // u16  r_index = u16( ResDesc.BindPoint + ((destination&1)? 0 : CTexture::rstVertex) );

        u16 r_index = u16(-1);

        if (destination & RC_dest_pixel)
        {
            r_index = u16(ResDesc.BindPoint + CTexture::rstPixel);
        }
        else if (destination & RC_dest_vertex)
        {
            r_index = u16(ResDesc.BindPoint + CTexture::rstVertex);
        }
        else if (destination & RC_dest_geometry)
        {
            r_index = u16(ResDesc.BindPoint + CTexture::rstGeometry);
        }
        else if (destination & RC_dest_hull)
        {
            r_index = u16(ResDesc.BindPoint + CTexture::rstHull);
        }
        else if (destination & RC_dest_domain)
        {
            r_index = u16(ResDesc.BindPoint + CTexture::rstDomain);
        }
        else if (destination & RC_dest_compute)
        {
            r_index = u16(ResDesc.BindPoint + CTexture::rstCompute);
        }
        else
        {
            VERIFY(0);
        }

        ref_constant C = get(ResDesc.Name, dx9compatibility ? type : u16(-1));
        if (!C)
        {
            C = table.emplace_back(xr_new<R_constant>()); //.g_constant_allocator.create();
            C->name = ResDesc.Name;
            C->destination = RC_dest_sampler;
            C->type = type;
            R_constant_load& L = C->samp;
            L.index = r_index;
            L.cls = type;
        }
        else
        {
            R_ASSERT(C->destination == RC_dest_sampler);
            R_ASSERT(C->type == type);
            R_constant_load& L = C->samp;
            R_ASSERT(L.index == r_index);
            R_ASSERT(L.cls == type);
        }
    }
    return TRUE;
}

IC u32 dest_to_shift_value(u32 destination)
{
    switch (destination & 0xFF)
    {
    case RC_dest_vertex: return RC_dest_vertex_cb_index_shift;
    case RC_dest_pixel: return RC_dest_pixel_cb_index_shift;
    case RC_dest_geometry: return RC_dest_geometry_cb_index_shift;
    case RC_dest_hull: return RC_dest_hull_cb_index_shift;
    case RC_dest_domain: return RC_dest_domain_cb_index_shift;
    case RC_dest_compute: return RC_dest_compute_cb_index_shift;
    default: FATAL("invalid enumeration for shader");
    }
    return 0;
}

IC u32 dest_to_cbuf_type(u32 destination)
{
    switch (destination & 0xFF)
    {
    case RC_dest_vertex: return CB_BufferVertexShader;
    case RC_dest_pixel: return CB_BufferPixelShader;
    case RC_dest_geometry: return CB_BufferGeometryShader;
    case RC_dest_hull: return CB_BufferHullShader;
    case RC_dest_domain: return CB_BufferDomainShader;
    case RC_dest_compute: return CB_BufferComputeShader;
    default: FATAL("invalid enumeration for shader");
    }
    return 0;
}

BOOL R_constant_table::parse(void* _desc, u32 destination)
{
    ID3DShaderReflection* pReflection = (ID3DShaderReflection*)_desc;

    D3D_SHADER_DESC ShaderDesc;
    pReflection->GetDesc(&ShaderDesc);

    if (ShaderDesc.ConstantBuffers)
    {
        for (int id = 0; id < R__NUM_CONTEXTS; ++id)
        {
            m_CBTable[id].reserve(ShaderDesc.ConstantBuffers);
        }
        //  Parse single constant table
        ID3DShaderReflectionConstantBuffer* pTable = 0;

        for (u16 iBuf = 0; iBuf < ShaderDesc.ConstantBuffers; ++iBuf)
        {
            pTable = pReflection->GetConstantBufferByIndex(iBuf);
            if (pTable)
            {
                // Get buffer description to check type
                D3D_SHADER_BUFFER_DESC bufferDesc;
                pTable->GetDesc(&bufferDesc);

                // Only process actual constant buffers, skip resource buffers
                // D3D_CT_CBUFFER = actual constant buffer
                // D3D_CT_TBUFFER = texture buffer (obsolete)
                // D3D_CT_RESOURCE_BIND_INFO = structured/byte address buffers (not cbuffers)
                if (bufferDesc.Type != D3D_CT_CBUFFER)
                {
                    // Skip structured buffers, byte address buffers, etc.
                    // These are handled by parseResources() as UAVs/SRVs
                    continue;
                }

                //  Encode buffer index into destination
                u32 updatedDest = destination;
                updatedDest |= iBuf << dest_to_shift_value(destination); /*((destination&RC_dest_pixel)
                    ? RC_dest_pixel_cb_index_shift : (destination&RC_dest_vertex)
                    ? RC_dest_vertex_cb_index_shift : RC_dest_geometry_cb_index_shift);*/

                //  Encode bind dest (pixel/vertex buffer) and bind point index
                u32 uiBufferIndex = iBuf;
                uiBufferIndex |= dest_to_cbuf_type(destination); /*(destination&RC_dest_pixel)
                     ? CB_BufferPixelShader : (destination&RC_dest_vertex)
                     ? CB_BufferVertexShader : CB_BufferGeometryShader;*/

                parseConstants(pTable, updatedDest);
                for (int id = 0; id < R__NUM_CONTEXTS; ++id)
                {
                    ref_cbuffer tempBuffer = RImplementation.Resources->_CreateConstantBuffer(id, pTable);
                    m_CBTable[id].push_back(cb_table_record(uiBufferIndex, tempBuffer));
                }
            }
        }
    }

    if (ShaderDesc.BoundResources)
    {
        parseResources(pReflection, ShaderDesc.BoundResources, destination);
    }

    std::sort(table.begin(), table.end(), [](const ref_constant& C1, const ref_constant& C2)
    {
        return xr_strcmp(C1->name, C2->name) < 0;
    });
    return TRUE;
}
} // namespace xray::render::RENDER_NAMESPACE
