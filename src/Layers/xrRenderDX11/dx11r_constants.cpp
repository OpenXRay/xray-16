#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderDX11/ResourceManager.h"
#include "xrCore/xrPool.h"
#include "Layers/xrRender/r_constants.h"
#include "Layers/xrRenderDX11/dx11ConstantBuffer.h"
#include "Layers/xrRender/Shaders/SlangReflectionWrapper.h"

namespace xray::render::fg
{
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

BOOL R_constant_table::parseSlangReflection(slang::ShaderReflection* reflection, u32 destination)
{
    if (!reflection)
        return FALSE;

    SlangReflectionWrapper wrapper(reflection);

    // Parse textures (ShaderResource category - t registers)
    auto textures = wrapper.GetTextures();
    for (const auto& tex : textures)
    {
        u16 r_index = u16(-1);

        // Apply stage offset based on destination
        if (destination & RC_dest_pixel)
            r_index = u16(tex.slot + CTexture::rstPixel);
        else if (destination & RC_dest_vertex)
            r_index = u16(tex.slot + CTexture::rstVertex);
        else if (destination & RC_dest_geometry)
            r_index = u16(tex.slot + CTexture::rstGeometry);
        else if (destination & RC_dest_hull)
            r_index = u16(tex.slot + CTexture::rstHull);
        else if (destination & RC_dest_domain)
            r_index = u16(tex.slot + CTexture::rstDomain);
        else if (destination & RC_dest_compute)
            r_index = u16(tex.slot + CTexture::rstCompute);
        else
        {
            VERIFY(0);
            continue;
        }

        ref_constant C = get(tex.name, u16(-1));
        if (!C)
        {
            C = table.emplace_back(xr_new<R_constant>());
            C->name = tex.name;
            C->destination = RC_dest_sampler;
            C->type = RC_dx11texture;
            R_constant_load& L = C->samp;
            L.index = r_index;
            L.cls = RC_dx11texture;
            Msg("  [parseSlangReflection] Added texture constant '%s' (type=RC_dx11texture, index=%u)", tex.name, r_index);
        }
        else
        {
            R_ASSERT(C->destination == RC_dest_sampler);
            R_ASSERT(C->type == RC_dx11texture);
            R_constant_load& L = C->samp;
            R_ASSERT(L.index == r_index);
            R_ASSERT(L.cls == RC_dx11texture);
        }
    }

    // Parse samplers (SamplerState category - s registers)
    auto samplers = wrapper.GetSamplers();
    for (const auto& samp : samplers)
    {
        u16 r_index = u16(-1);

        // Apply stage offset based on destination
        if (destination & RC_dest_pixel)
            r_index = u16(samp.slot + CTexture::rstPixel);
        else if (destination & RC_dest_vertex)
            r_index = u16(samp.slot + CTexture::rstVertex);
        else if (destination & RC_dest_geometry)
            r_index = u16(samp.slot + CTexture::rstGeometry);
        else if (destination & RC_dest_hull)
            r_index = u16(samp.slot + CTexture::rstHull);
        else if (destination & RC_dest_domain)
            r_index = u16(samp.slot + CTexture::rstDomain);
        else if (destination & RC_dest_compute)
            r_index = u16(samp.slot + CTexture::rstCompute);
        else
        {
            VERIFY(0);
            continue;
        }

        ref_constant C = get(samp.name, u16(-1));
        if (!C)
        {
            C = table.emplace_back(xr_new<R_constant>());
            C->name = samp.name;
            C->destination = RC_dest_sampler;
            C->type = RC_sampler;
            R_constant_load& L = C->samp;
            L.index = r_index;
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

    // Parse UAVs (UnorderedAccess category - u registers)
    auto uavs = wrapper.GetUAVs();
    for (const auto& uav : uavs)
    {
        u16 r_index = u16(-1);

        // Apply stage offset based on destination
        if (destination & RC_dest_pixel)
            r_index = u16(uav.slot + CTexture::rstPixel);
        else if (destination & RC_dest_vertex)
            r_index = u16(uav.slot + CTexture::rstVertex);
        else if (destination & RC_dest_geometry)
            r_index = u16(uav.slot + CTexture::rstGeometry);
        else if (destination & RC_dest_hull)
            r_index = u16(uav.slot + CTexture::rstHull);
        else if (destination & RC_dest_domain)
            r_index = u16(uav.slot + CTexture::rstDomain);
        else if (destination & RC_dest_compute)
            r_index = u16(uav.slot + CTexture::rstCompute);
        else
        {
            VERIFY(0);
            continue;
        }

        ref_constant C = get(uav.name, u16(-1));
        if (!C)
        {
            C = table.emplace_back(xr_new<R_constant>());
            C->name = uav.name;
            C->destination = RC_dest_sampler;
            C->type = RC_dx11UAV;
            R_constant_load& L = C->samp;
            L.index = r_index;
            L.cls = RC_dx11UAV;
        }
        else
        {
            R_ASSERT(C->destination == RC_dest_sampler);
            R_ASSERT(C->type == RC_dx11UAV);
            R_constant_load& L = C->samp;
            R_ASSERT(L.index == r_index);
            R_ASSERT(L.cls == RC_dx11UAV);
        }
    }

    // Parse constant buffers (ConstantBuffer category - b registers)
    auto constantBuffers = wrapper.GetConstantBuffers();
    if (!constantBuffers.empty())
    {
        for (int id = 0; id < R__NUM_CONTEXTS; ++id)
        {
            m_CBTable[id].reserve(constantBuffers.size());
        }

        for (const auto& cb : constantBuffers)
        {
            // Slang gives us the ACTUAL bind point directly!
            u32 actualBindPoint = cb.slot;
            u32 bufferIndex = actualBindPoint | dest_to_cbuf_type(destination);

            for (int id = 0; id < R__NUM_CONTEXTS; ++id)
            {
                ref_cbuffer tempBuffer = RImplementation.Resources->_CreateConstantBufferSlang(id, cb.name, cb.size);
                m_CBTable[id].push_back(cb_table_record(bufferIndex, tempBuffer));
            }
        }
    }

    // Sort the constant table by name
    std::sort(table.begin(), table.end(), [](const ref_constant& C1, const ref_constant& C2)
    {
        return xr_strcmp(C1->name, C2->name) < 0;
    });

    return TRUE;
}
} // namespace xray::render::fg
