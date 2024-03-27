#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/r_constants_cache.h"

template<>
dx11ConstantBuffer& R_constants::GetCBuffer<R_constants::BT_PixelBuffer>(R_constant* C) const
{
    //	Decode index
    int iBufferIndex = (C->destination & RC_dest_pixel_cb_index_mask) >> RC_dest_pixel_cb_index_shift;

    VERIFY(iBufferIndex < CBackend::MaxCBuffers);
    VERIFY(cmd_list.m_aPixelConstants[iBufferIndex]);
    return *cmd_list.m_aPixelConstants[iBufferIndex];
}

template<>
dx11ConstantBuffer& R_constants::GetCBuffer<R_constants::BT_VertexBuffer>(R_constant* C) const
{
    //	Decode index
    int iBufferIndex = (C->destination & RC_dest_vertex_cb_index_mask) >> RC_dest_vertex_cb_index_shift;

    VERIFY(iBufferIndex < CBackend::MaxCBuffers);
    VERIFY(cmd_list.m_aVertexConstants[iBufferIndex]);
    return *cmd_list.m_aVertexConstants[iBufferIndex];
}

template<>
dx11ConstantBuffer& R_constants::GetCBuffer<R_constants::BT_GeometryBuffer>(R_constant* C) const
{
    //	Decode index
    int iBufferIndex = (C->destination & RC_dest_geometry_cb_index_mask) >> RC_dest_geometry_cb_index_shift;

    VERIFY(iBufferIndex < CBackend::MaxCBuffers);
    VERIFY(cmd_list.m_aGeometryConstants[iBufferIndex]);
    return *cmd_list.m_aGeometryConstants[iBufferIndex];
}

template<>
dx11ConstantBuffer& R_constants::GetCBuffer<R_constants::BT_ComputeBuffer>(R_constant* C) const
{
    //	Decode index
    int iBufferIndex = (C->destination & RC_dest_compute_cb_index_mask) >> RC_dest_compute_cb_index_shift;

    VERIFY(iBufferIndex < CBackend::MaxCBuffers);
    VERIFY(cmd_list.m_aComputeConstants[iBufferIndex]);
    return *cmd_list.m_aComputeConstants[iBufferIndex];
}

template<>
dx11ConstantBuffer& R_constants::GetCBuffer<R_constants::BT_HullBuffer>(R_constant* C) const
{
    //	Decode index
    int iBufferIndex = (C->destination & RC_dest_hull_cb_index_mask) >> RC_dest_hull_cb_index_shift;

    VERIFY(iBufferIndex < CBackend::MaxCBuffers);
    VERIFY(cmd_list.m_aHullConstants[iBufferIndex]);
    return *cmd_list.m_aHullConstants[iBufferIndex];
}

template<>
dx11ConstantBuffer& R_constants::GetCBuffer<R_constants::BT_DomainBuffer>(R_constant* C) const
{
    //	Decode index
    int iBufferIndex = (C->destination & RC_dest_domain_cb_index_mask) >> RC_dest_domain_cb_index_shift;

    VERIFY(iBufferIndex < CBackend::MaxCBuffers);
    VERIFY(cmd_list.m_aDomainConstants[iBufferIndex]);
    return *cmd_list.m_aDomainConstants[iBufferIndex];
}

void R_constants::flush_cache()
{
    const auto context_id = cmd_list.context_id; // TODO: constant buffer should be encapsulated, so no ctx ID needed

    for (int i = 0; i < CBackend::MaxCBuffers; ++i)
    {
        if (cmd_list.m_aVertexConstants[i])
            cmd_list.m_aVertexConstants[i]->Flush(context_id);

        if (cmd_list.m_aPixelConstants[i])
            cmd_list.m_aPixelConstants[i]->Flush(context_id);

        if (cmd_list.m_aGeometryConstants[i])
            cmd_list.m_aGeometryConstants[i]->Flush(context_id);

        if (cmd_list.m_aHullConstants[i])
            cmd_list.m_aHullConstants[i]->Flush(context_id);

        if (cmd_list.m_aDomainConstants[i])
            cmd_list.m_aDomainConstants[i]->Flush(context_id);

        if (cmd_list.m_aComputeConstants[i])
            cmd_list.m_aComputeConstants[i]->Flush(context_id);
    }

#if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
    ID3DBuffer* tempBuffer[CBackend::MaxCBuffers] = {};
    std::vector<u32> tempBufferNum(CBackend::MaxCBuffers);
    std::vector<u32> tempBufferSize(CBackend::MaxCBuffers);
    u32 uiMin = 0, uiMax = 0;
    {
        bool hasConstantBuffer = false;
        
        for (u32 i = uiMin; i < CBackend::MaxCBuffers; ++i)
        {
            if (cmd_list.m_aPixelConstants[i])
            {
                tempBuffer[i] = cmd_list.m_aPixelConstants[i]->GetBuffer();
                tempBufferNum[i] = cmd_list.m_aPixelConstants[i]->GetBufferNum();
                tempBufferSize[i] = cmd_list.m_aPixelConstants[i]->GetBufferSize();
                if (!hasConstantBuffer)
                    uiMin = i;
                hasConstantBuffer = true; 
                uiMax = i + 1;
            }
            else
            {
                tempBuffer[i] = nullptr;
            }
        }
      
        if (hasConstantBuffer)
        {
#if USE_DX11
            HW.pContext1->PSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#else
            HW.get_context(context_id)->PSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#endif
        }
    }

    {
        bool hasConstantBuffer = false;

        for (u32 i = uiMin; i < CBackend::MaxCBuffers; ++i)
        {
            if (cmd_list.m_aVertexConstants[i])
            {
                tempBuffer[i] = cmd_list.m_aVertexConstants[i]->GetBuffer();
                tempBufferNum[i] = cmd_list.m_aVertexConstants[i]->GetBufferNum();
                tempBufferSize[i] = cmd_list.m_aVertexConstants[i]->GetBufferSize();
                if (!hasConstantBuffer)
                    uiMin = i;
                hasConstantBuffer = true; 
                uiMax = i + 1;
            }
            else
            {
                tempBuffer[i] = nullptr;
            }
        }
        
        if (hasConstantBuffer)
        {
#if USE_DX11
            HW.pContext1->VSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#else
            HW.get_context(context_id)->VSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#endif
        }
    }

    {
        bool hasConstantBuffer = false;

        for (u32 i = uiMin; i < CBackend::MaxCBuffers; ++i)
        {
            if (cmd_list.m_aGeometryConstants[i])
            {
                tempBuffer[i] = cmd_list.m_aGeometryConstants[i]->GetBuffer();
                tempBufferNum[i] = cmd_list.m_aGeometryConstants[i]->GetBufferNum();
                tempBufferSize[i] = cmd_list.m_aGeometryConstants[i]->GetBufferSize();
                if (!hasConstantBuffer)
                    uiMin = i;
                hasConstantBuffer = true; 
                uiMax = i + 1;
            }
            else
            {
                tempBuffer[i] = nullptr;
            }
        }
      
        if (hasConstantBuffer)     
        {
#if USE_DX11
            HW.pContext1->GSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#else
            HW.get_context(context_id)->GSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#endif
        }
    }

    {
        bool hasConstantBuffer = false;

        for (u32 i = uiMin; i < CBackend::MaxCBuffers; ++i)
        {
            if (cmd_list.m_aHullConstants[i])
            {
                tempBuffer[i] = cmd_list.m_aHullConstants[i]->GetBuffer();
                tempBufferNum[i] = cmd_list.m_aHullConstants[i]->GetBufferNum();
                tempBufferSize[i] = cmd_list.m_aHullConstants[i]->GetBufferSize();
                if (!hasConstantBuffer)
                    uiMin = i;
                hasConstantBuffer = true;
                uiMax = i + 1;
            }
            else
            {
                tempBuffer[i] = nullptr;
            }
        }
       
        if (hasConstantBuffer)
        {
#if USE_DX11
            HW.pContext1->HSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#else
            HW.get_context(context_id)->HSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#endif
        }
    }

    {
        bool hasConstantBuffer = false;

        for (u32 i = 0; i < CBackend::MaxCBuffers; ++i)
        {
            if (cmd_list.m_aDomainConstants[i])
            {
                tempBuffer[i] = cmd_list.m_aDomainConstants[i]->GetBuffer();
                tempBufferNum[i] = cmd_list.m_aDomainConstants[i]->GetBufferNum();
                tempBufferSize[i] = cmd_list.m_aDomainConstants[i]->GetBufferSize();
                if (!hasConstantBuffer)
                    uiMin = i;
                hasConstantBuffer = true;
                uiMax = i + 1;
            }
            else
            {
                tempBuffer[i] = nullptr;
            }
        }
       
        if (hasConstantBuffer)
        {
#if USE_DX11
            HW.pContext1->DSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#else
            HW.get_context(context_id)->DSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#endif
        }
    }

    {
        bool hasConstantBuffer = false;

        for (u32 i = 0; i < CBackend::MaxCBuffers; ++i)
        {
            if (cmd_list.m_aComputeConstants[i])
            {
                tempBuffer[i] = cmd_list.m_aComputeConstants[i]->GetBuffer();
                tempBufferNum[i] = cmd_list.m_aComputeConstants[i]->GetBufferNum();
                tempBufferSize[i] = cmd_list.m_aComputeConstants[i]->GetBufferSize();
                if (!hasConstantBuffer)
                    uiMin = i;
                hasConstantBuffer = true;
                uiMax = i + 1;
            }
            else
            {
                tempBuffer[i] = nullptr;
            }
        }
        
        if (hasConstantBuffer)
        {
#if USE_DX11
            HW.pContext1->CSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#else
            HW.get_context(context_id)->CSSetConstantBuffers1(uiMin, uiMax - uiMin, &tempBuffer[uiMin], tempBufferNum.data(), tempBufferSize.data());
#endif
        }
    }
#endif
}

/*
void R_constants::flush_cache()
{
    if (a_pixel.b_dirty)
    {
        // fp
        R_constant_array::t_f&	F	= a_pixel.c_f;
        {
            //if (F.r_lo() <= 32) //. hack
            {
                void	*pBuffer;
                const int iVectorElements = 4;
                const int iVectorNumber = 256;
                RCache.m_pPixelConstants->Map(D3Dxx_MAP_WRITE_DISCARD, 0, &pBuffer);
                CopyMemory(pBuffer, F.access(0), iVectorNumber*iVectorElements*sizeof(float));
                RCache.m_pPixelConstants->Unmap();
            }
        }
        a_pixel.b_dirty		= false;
    }
    if (a_vertex.b_dirty)
    {
        // fp
        R_constant_array::t_f&	F	= a_vertex.c_f;
        {
            u32		count		= F.r_hi()-F.r_lo();
            if (count)			{
#ifdef DEBUG
                if (F.r_hi() > HW.Caps.geometry.dwRegisters)
                {
                    xrDebug::Fatal(DEBUG_INFO,"Internal error setting VS-constants: overflow\nregs[%d],hi[%d]",
                        HW.Caps.geometry.dwRegisters,F.r_hi()
                        );
                }
                PGO		(Msg("PGO:V_CONST:%d",count));
#endif
                {
                    void	*pBuffer;
                    const int iVectorElements = 4;
                    const int iVectorNumber = 256;
                    RCache.m_pVertexConstants->Map(D3Dxx_MAP_WRITE_DISCARD, 0, &pBuffer);
                    CopyMemory(pBuffer, F.access(0), iVectorNumber*iVectorElements*sizeof(float));
                    RCache.m_pVertexConstants->Unmap();
                }
                F.flush	();
            }
        }
        a_vertex.b_dirty	= false;
    }
}
*/
