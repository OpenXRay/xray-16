/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#include "StdAfx.h"
#include "DX12Shader.hpp"

#include <d3d11shader.h>
#include <d3dcompiler.h>

namespace DX12
{
    namespace
    {

        u32 MakeHash(ReflectedBindings& bindings)
        {
            const ReflectedBindingRangeList* lists[] =
            {
                &bindings.m_ConstantBuffers,
                &bindings.m_InputResources,
                &bindings.m_OutputResources,
                &bindings.m_Samplers
            };

            hash::crc32 hash = 0;

            for (u32 i = 0; i < DX12_ARRAY_SIZE(lists); ++i)
            {
                const ReflectedBindingRangeList& list = *lists[i];
                if (list.m_DescriptorCount)
                {
                    hash.Add(&i, sizeof(u32));
                    hash.Add(list.m_Ranges.data(), list.m_Ranges.size() * sizeof(ReflectedBindingRange));
                }
            }

            return hash;
        }

        void AppendResourceToRanges(
            ReflectedBindingRangeList& bindings,
            u8 shaderRegister,
            u8 count,
            u8 type,
            u8 dimension,
            bool bUsed = true,
            bool bShared = false,
            bool bMergeable = true)
        {
            u32 rangeIdx = 0;
            u32 rangeCount = bindings.m_Ranges.size();

            // Attempts to merge a resource into an existing range. We can do this if
            // the resource is marked as mergeable, and it's not shared across different
            // shader stages.
            bool bMerged = false;

            if (bMergeable && !bShared)
            {
                for (; rangeIdx < rangeCount; ++rangeIdx)
                {
                    ReflectedBindingRange& r = bindings.m_Ranges[rangeIdx];
                    if (r.m_bMergeable || r.m_bShared || (r.m_bUsed != bUsed))
                    {
                        continue; // Non-Mergeable
                    }

                    // Can we join new range with an existing one?
                    if (r.m_ShaderRegister + r.m_Count == shaderRegister)
                    {
#ifdef GFX_DEBUG
                        for (size_t j = 0; j < count; ++j)
                        {
                            r.m_Types[r.m_Count + j] = type;
                            r.m_Dimensions[r.m_Count + j] = dimension;
                        }
#endif

                        r.m_Count += count;
                        bMerged = true;
                        break;
                    }
                }
            }

            if (!bMerged)
            {
                bindings.m_Ranges.push_back(ReflectedBindingRange(shaderRegister, count, type, dimension, bUsed, bShared, bMergeable));
            }

            bindings.m_DescriptorCount += count;
        }
    }

    Shader::Shader(Device* device)
        : DeviceObject(device)
    {}

    Shader::~Shader()
    {}

    Shader* Shader::CreateFromD3D11(Device* device, const D3D12_SHADER_BYTECODE& byteCode)
    {
        ID3D11ShaderReflection* shaderReflection = NULL;

        // Reflect shader internals
        if (S_OK != D3DReflect(byteCode.pShaderBytecode, byteCode.BytecodeLength, IID_ID3D11ShaderReflection, (void**)&shaderReflection))
        {
            DX12_ASSERT(0, "Could not do a shader reflection!");
            return NULL;
        }

        D3D11_SHADER_DESC desc11;
        shaderReflection->GetDesc(&desc11);

        Shader* result = DX12::PassAddRef(new Shader(device));
        result->m_Bytecode = byteCode;

        for (UINT i = 0; i < desc11.BoundResources; ++i)
        {
            D3D11_SHADER_INPUT_BIND_DESC bindDesc11;
            shaderReflection->GetResourceBindingDesc(i, &bindDesc11);

            bool bBindPointUsed = true;
            bool bBindingSharable =
                bindDesc11.BindPoint == eConstantBufferShaderSlot_PerMaterial ||
                bindDesc11.BindPoint == eConstantBufferShaderSlot_PerPass ||
                bindDesc11.BindPoint == eConstantBufferShaderSlot_PerView ||
                bindDesc11.BindPoint == eConstantBufferShaderSlot_PerFrame;

            bool bBindingMergeable = bindDesc11.BindPoint > eConstantBufferShaderSlot_PerInstanceLegacy;

            switch (bindDesc11.Type)
            {
            case D3D10_SIT_CBUFFER:
                DX12_ASSERT(bindDesc11.BindCount == 1, "Arrays of ConstantBuffers are not allowed!");

                if (bindDesc11.BindCount == 1)
                {
                    ID3D11ShaderReflectionConstantBuffer* constantBuffer = shaderReflection->GetConstantBufferByName(bindDesc11.Name);
                    D3D11_SHADER_BUFFER_DESC constantBufferDesc;
                    UINT variableUsedCount = 0;

                    constantBuffer->GetDesc(&constantBufferDesc);
                    for (UINT j = 0; j < constantBufferDesc.Variables; j++)
                    {
                        ID3D11ShaderReflectionVariable* variable = constantBuffer->GetVariableByIndex(j);
                        D3D11_SHADER_VARIABLE_DESC variableDesc;
                        variable->GetDesc(&variableDesc);

                        variableUsedCount += (variableDesc.uFlags & D3D10_SVF_USED);
                    }

                    bBindPointUsed = (variableUsedCount > 0);
                }

                // We need separate descriptor tables for dynamic CB's
                AppendResourceToRanges(
                    result->m_Bindings.m_ConstantBuffers,
                    bindDesc11.BindPoint,
                    bindDesc11.BindCount,
                    static_cast<UINT8>(bindDesc11.Type),
                    static_cast<UINT8>(bindDesc11.Dimension),
                    bBindPointUsed,
                    bBindingSharable,
                    bBindingMergeable);
                break;

            // ID3D12Device::CreateGraphicsPipelineState: SRV or UAV root descriptors can only be Raw or Structured buffers.
            case D3D10_SIT_TEXTURE:
            case D3D10_SIT_TBUFFER:
                AppendResourceToRanges(
                    result->m_Bindings.m_InputResources,
                    bindDesc11.BindPoint,
                    bindDesc11.BindCount,
                    static_cast<UINT8>(bindDesc11.Type),
                    static_cast<UINT8>(bindDesc11.Dimension));
                break;
            case D3D11_SIT_STRUCTURED:
            case D3D11_SIT_BYTEADDRESS:
                AppendResourceToRanges(
                    result->m_Bindings.m_InputResources,
                    bindDesc11.BindPoint,
                    bindDesc11.BindCount,
                    static_cast<UINT8>(bindDesc11.Type),
                    static_cast<UINT8>(bindDesc11.Dimension),
                    bBindPointUsed);
                break;

            case D3D11_SIT_UAV_RWTYPED:
                AppendResourceToRanges(
                    result->m_Bindings.m_OutputResources,
                    bindDesc11.BindPoint,
                    bindDesc11.BindCount,
                    static_cast<UINT8>(bindDesc11.Type),
                    static_cast<UINT8>(bindDesc11.Dimension));
                break;
            case D3D11_SIT_UAV_RWSTRUCTURED:
            case D3D11_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
            case D3D11_SIT_UAV_RWBYTEADDRESS:
            case D3D11_SIT_UAV_APPEND_STRUCTURED:
            case D3D11_SIT_UAV_CONSUME_STRUCTURED:
                AppendResourceToRanges(result->m_Bindings.m_OutputResources,
                    bindDesc11.BindPoint,
                    bindDesc11.BindCount,
                    static_cast<UINT8>(bindDesc11.Type),
                    static_cast<UINT8>(bindDesc11.Dimension),
                    bBindPointUsed);
                break;

            case D3D10_SIT_SAMPLER:
                DX12_ASSERT(bindDesc11.BindCount == 1, "Arrays of SamplerStates are not allowed!");
                AppendResourceToRanges(
                    result->m_Bindings.m_Samplers,
                    bindDesc11.BindPoint,
                    bindDesc11.BindCount,
                    static_cast<UINT8>(bindDesc11.Type),
                    static_cast<UINT8>(bindDesc11.Dimension));
                break;
            }
        }

        result->m_BindingHash = MakeHash(result->m_Bindings);
        return result;
    }
}
