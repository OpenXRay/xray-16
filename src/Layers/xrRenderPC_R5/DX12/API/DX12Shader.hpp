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
#pragma once
#ifndef __DX12SHADER__
#define __DX12SHADER__

#include "DX12.hpp"

namespace DX12
{
    enum EShaderStage : int8_t
    {
        // Graphics pipeline
        ESS_Vertex,
        ESS_Hull,
        ESS_Domain,
        ESS_Geometry,
        ESS_Pixel,

        // Compute pipeline
        ESS_Compute,

        // Complete pipeline
        ESS_All = ESS_Compute,

        ESS_LastWithoutCompute = ESS_Compute,
        ESS_Num = ESS_Compute + 1,
        ESS_None = -1,
        ESS_First = ESS_Vertex
    };

    enum EConstantBufferShaderSlot : uint8_t
    {
        // Reflected constant buffers:
        //    These are built by the shader system using the parameter system at
        //    shader compilation time and their format varies based on the permutation.
        //
        // These are deprecated and are being replaced by well-defined constant buffer definitions in HLSL.
        eConstantBufferShaderSlot_PerBatch = 0,
        eConstantBufferShaderSlot_PerInstanceLegacy = 1,
        eConstantBufferShaderSlot_PerMaterial = 2,
        eConstantBufferShaderSlot_ReflectedCount =
        eConstantBufferShaderSlot_PerMaterial + 1, // This slot is used only for counting. It's not a real binding
                                                       // value. !Reflected constant buffers

        eConstantBufferShaderSlot_SPIIndex = 3,
        eConstantBufferShaderSlot_PerInstance = 4,
        eConstantBufferShaderSlot_SPI = 5,
        eConstantBufferShaderSlot_SkinQuat = 6,
        eConstantBufferShaderSlot_SkinQuatPrev = 7,
        eConstantBufferShaderSlot_PerSubPass = 8,
        eConstantBufferShaderSlot_PerPass = 9,
        eConstantBufferShaderSlot_PerView = 10,
        eConstantBufferShaderSlot_PerFrame = 11,
        // OpenGLES 3.X guarantees only 12 uniform slots for VS and PS.
        eConstantBufferShaderSlot_Count
    };

    struct ReflectedBindingRange
    {
        u8 m_ShaderRegister;
        u8 m_Count;

        // Whether the bind point is actually used in the shader. Ideally, this is compiled out, but in order to support
        // lower tier hardware we have to define a root parameter and bind a null descriptor for it.
        u8 m_bUsed : 1;

        // Shared bindings are marked as visible across all shader stages. Practically, this means the binding
        // is not merged and will be de-duplicated when the final layout is built. Candidates for sharing are constant
        // buffers that are accessed by every stage and have a well-defined layout across all stages.
        u8 m_bShared : 1;

        // We currently merge any range where the layout contents aren't reflected by the shader. Practically,
        // this means that any constant buffer that's reflected gets placed into a root constant buffer parameter.
        u8 m_bMergeable : 1;

#ifdef GFX_DEBUG
        u8 m_Types[56];
        u8 m_Dimensions[56];
#endif

        ReflectedBindingRange()
            : m_ShaderRegister(0)
            , m_Count(0)
            , m_bUsed(true)
            , m_bShared(false)
            , m_bMergeable(true)
        {}

        ReflectedBindingRange(
            u8 shaderRegister,
            u8 count,
            u8 type,
            u8 dimension,
            bool bUsed,
            bool bShared,
            bool bMergeable)
            : m_ShaderRegister(shaderRegister)
            , m_Count(count)
            , m_bUsed(bUsed)
            , m_bShared(bShared)
            , m_bMergeable(bMergeable)
        {
#ifdef GFX_DEBUG
            for (u32 i = 0; i < count; ++i)
            {
                m_Types[i] = type;
                m_Dimensions[i] = dimension;
            }
#endif
        }
    };

    struct ReflectedBindingRangeList
    {
        std::vector<ReflectedBindingRange> m_Ranges;
        u32 m_DescriptorCount;

        ReflectedBindingRangeList()
            : m_DescriptorCount(0)
        {}
    };

    ////////////////////////////////////////////////////////////////////////////

    struct ReflectedBindings
    {
        ReflectedBindingRangeList m_ConstantBuffers;    // CBV
        ReflectedBindingRangeList m_InputResources;     // SRV
        ReflectedBindingRangeList m_OutputResources;    // UAV
        ReflectedBindingRangeList m_Samplers;           // SMP
    };

    ////////////////////////////////////////////////////////////////////////////

    class Shader : public DeviceObject
    {
    public:
        // Create new shader using DX11 reflection interface
        static Shader* CreateFromD3D11(Device* device, const D3D12_SHADER_BYTECODE& byteCode);

        Shader(Device* device);

        const D3D12_SHADER_BYTECODE& GetBytecode() const
        {
            return m_Bytecode;
        }

        const ReflectedBindings& GetReflectedBindings() const
        {
            return m_Bindings;
        }

        const u32 GetReflectedBindingHash() const
        {
            return m_BindingHash;
        }

    protected:
        virtual ~Shader();

    private:
        D3D12_SHADER_BYTECODE m_Bytecode;
        ReflectedBindings m_Bindings;
        u32 m_BindingHash;
    };
}

#endif // __DX12SHADER__
