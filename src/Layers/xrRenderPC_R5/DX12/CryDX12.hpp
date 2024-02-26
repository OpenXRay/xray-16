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

#include "CryDX12Legacy.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "DX12/API/DX12.hpp"
#include "DX12/API/DX12PSO.hpp"
#include "DX12/Misc/SCryDX11PipelineState.hpp"

#include "DX12/GI/CCryDX12GIFactory.hpp"
#include "DX12/GI/CCryDX12SwapChain.hpp"
#include "DX12/Device/CCryDX12Device.hpp"
#include "DX12/Device/CCryDX12DeviceContext.hpp"

typedef IDXGIDevice3            DXGIDevice;
typedef IDXGIAdapter3           DXGIAdapter;
typedef CCryDX12GIFactory       DXGIFactory;
typedef IDXGIOutput4            DXGIOutput;
typedef CCryDX12SwapChain       DXGISwapChain;
typedef CCryDX12Device          D3DDevice;
typedef CCryDX12DeviceContext   D3DDeviceContext;

HRESULT WINAPI D3DReflectDXILorDXBC(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData, _In_ SIZE_T SrcDataSize,
    _In_ REFIID pInterface, _Out_ void** ppReflector);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI DX12CreateDXGIFactory(REFIID riid, void** ppFactory);

HRESULT WINAPI DX12CreateDevice(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext);
