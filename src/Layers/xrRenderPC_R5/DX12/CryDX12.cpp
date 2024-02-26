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
#include "CryDX12.hpp"

#include "GI/CCryDX12GIFactory.hpp"
#include "Device/CCryDX12Device.hpp"
#include "Device/CCryDX12DeviceContext.hpp"

#ifdef WIN32
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#if USE_DXC
#pragma comment(lib, "dxcompiler.lib")
#endif
#endif

HRESULT WINAPI DX12CreateDXGIFactory(REFIID riid, void** ppFactory)
{
    *ppFactory = CCryDX12GIFactory::Create();
    return *ppFactory ? 0 : -1;
}

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
    ID3D11DeviceContext** ppImmediateContext)
{
    *ppDevice = CCryDX12Device::Create(pAdapter, pFeatureLevel);

    if (!*ppDevice)
    {
        return -1;
    }

    (*ppDevice)->GetImmediateContext(ppImmediateContext);

    if (!*ppImmediateContext)
    {
        return -1;
    }

    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
#if USE_DXC
#include <atlconv.h>
#include <atlcomcli.h>
using namespace ATL;

#define DXC_FAILED(hr) (((HRESULT)(hr)) < 0)
#define IFR(x)                                                                                                         \
    {                                                                                                                  \
        HRESULT __hr = (x);                                                                                            \
        if (DXC_FAILED(__hr))                                                                                          \
            return __hr;                                                                                               \
    }
#define IFT(x)                                                                                                         \
    {                                                                                                                  \
        HRESULT __hr = (x);                                                                                            \
        if (DXC_FAILED(__hr))                                                                                          \
            return E_FAIL;                                                                                             \
    }

#include "Includes/dxc/dxcapi.h"

namespace hlsl
{

#define DXIL_FOURCC(ch0, ch1, ch2, ch3)                                                                                \
    ((uint32_t)(uint8_t)(ch0) | (uint32_t)(uint8_t)(ch1) << 8 | (uint32_t)(uint8_t)(ch2) << 16 |                       \
        (uint32_t)(uint8_t)(ch3) << 24)

enum DxilFourCC
{
    DFCC_Container = DXIL_FOURCC('D', 'X', 'B', 'C'), // for back-compat with tools that look for DXBC containers
    DFCC_ResourceDef = DXIL_FOURCC('R', 'D', 'E', 'F'),
    DFCC_InputSignature = DXIL_FOURCC('I', 'S', 'G', '1'),
    DFCC_OutputSignature = DXIL_FOURCC('O', 'S', 'G', '1'),
    DFCC_PatchConstantSignature = DXIL_FOURCC('P', 'S', 'G', '1'),
    DFCC_ShaderStatistics = DXIL_FOURCC('S', 'T', 'A', 'T'),
    DFCC_ShaderDebugInfoDXIL = DXIL_FOURCC('I', 'L', 'D', 'B'),
    DFCC_ShaderDebugName = DXIL_FOURCC('I', 'L', 'D', 'N'),
    DFCC_FeatureInfo = DXIL_FOURCC('S', 'F', 'I', '0'),
    DFCC_PrivateData = DXIL_FOURCC('P', 'R', 'I', 'V'),
    DFCC_RootSignature = DXIL_FOURCC('R', 'T', 'S', '0'),
    DFCC_DXIL = DXIL_FOURCC('D', 'X', 'I', 'L'),
    DFCC_PipelineStateValidation = DXIL_FOURCC('P', 'S', 'V', '0'),
    DFCC_RuntimeData = DXIL_FOURCC('R', 'D', 'A', 'T'),
    DFCC_ShaderHash = DXIL_FOURCC('H', 'A', 'S', 'H'),
    DFCC_ShaderSourceInfo = DXIL_FOURCC('S', 'R', 'C', 'I'),
    DFCC_ShaderPDBInfo = DXIL_FOURCC('P', 'D', 'B', 'I'),
    DFCC_CompilerVersion = DXIL_FOURCC('V', 'E', 'R', 'S'),
};

} // namespace hlsl

#include <d3dcompiler.h>
#include <vector>
#include <string>

HRESULT CreateLibrary(IDxcLibrary** pLibrary)
{
    return DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)pLibrary);
}

HRESULT CreateCompiler(IDxcCompiler** ppCompiler)
{
    return DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)ppCompiler);
}

HRESULT CreateContainerReflection(IDxcContainerReflection** ppReflection)
{
    return DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), (void**)ppReflection);
}

HRESULT CompileFromBlob(IDxcBlobEncoding* pSource, LPCWSTR pSourceName, const D3D_SHADER_MACRO* pDefines,
    IDxcIncludeHandler* pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs)
{
    CComPtr<IDxcCompiler> compiler;
    CComPtr<IDxcOperationResult> operationResult;
    HRESULT hr;

    // Upconvert legacy targets
    char Target[7] = "?s_6_0";
    Target[6] = 0;
    if (pTarget[3] < '6')
    {
        Target[0] = pTarget[0];
        pTarget = Target;
    }

    try
    {
        CA2W pEntrypointW(pEntrypoint);
        CA2W pTargetProfileW(pTarget);
        std::vector<std::wstring> defineValues;
        std::vector<DxcDefine> defines;
        if (pDefines)
        {
            CONST D3D_SHADER_MACRO* pCursor = pDefines;

            // Convert to UTF-16.
            while (pCursor->Name)
            {
                defineValues.push_back(std::wstring(CA2W(pCursor->Name)));
                if (pCursor->Definition)
                    defineValues.push_back(std::wstring(CA2W(pCursor->Definition)));
                else
                    defineValues.push_back(std::wstring());
                ++pCursor;
            }

            // Build up array.
            pCursor = pDefines;
            size_t i = 0;
            while (pCursor->Name)
            {
                defines.push_back(DxcDefine{defineValues[i++].c_str(), defineValues[i++].c_str()});
                ++pCursor;
            }
        }

        std::vector<LPCWSTR> arguments;
        // /Gec, /Ges Not implemented:
        // if(Flags1 & D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY) arguments.push_back(L"/Gec");
        // if(Flags1 & D3DCOMPILE_ENABLE_STRICTNESS) arguments.push_back(L"/Ges");
        if (Flags1 & D3DCOMPILE_IEEE_STRICTNESS)
            arguments.push_back(L"/Gis");
        if (Flags1 & D3DCOMPILE_OPTIMIZATION_LEVEL2)
        {
            switch (Flags1 & D3DCOMPILE_OPTIMIZATION_LEVEL2)
            {
            case D3DCOMPILE_OPTIMIZATION_LEVEL0: arguments.push_back(L"/O0"); break;
            case D3DCOMPILE_OPTIMIZATION_LEVEL2: arguments.push_back(L"/O2"); break;
            case D3DCOMPILE_OPTIMIZATION_LEVEL3: arguments.push_back(L"/O3"); break;
            }
        }
        // Currently, /Od turns off too many optimization passes, causing incorrect DXIL to be generated.
        // Re-enable once /Od is implemented properly:
        // if(Flags1 & D3DCOMPILE_SKIP_OPTIMIZATION) arguments.push_back(L"/Od");
        if (Flags1 & D3DCOMPILE_DEBUG)
            arguments.push_back(L"/Zi");
        if (Flags1 & D3DCOMPILE_PACK_MATRIX_ROW_MAJOR)
            arguments.push_back(L"/Zpr");
        if (Flags1 & D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR)
            arguments.push_back(L"/Zpc");
        if (Flags1 & D3DCOMPILE_AVOID_FLOW_CONTROL)
            arguments.push_back(L"/Gfa");
        if (Flags1 & D3DCOMPILE_PREFER_FLOW_CONTROL)
            arguments.push_back(L"/Gfp");
        // We don't implement this:
        // if(Flags1 & D3DCOMPILE_PARTIAL_PRECISION) arguments.push_back(L"/Gpp");
        if (Flags1 & D3DCOMPILE_RESOURCES_MAY_ALIAS)
            arguments.push_back(L"/res_may_alias");

        IFR(CreateCompiler(&compiler));
        IFR(compiler->Compile(pSource, pSourceName, pEntrypointW, pTargetProfileW, arguments.data(),
            (UINT)arguments.size(), defines.data(), (UINT)defines.size(), pInclude, &operationResult));
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    catch (const CAtlException& err)
    {
        return err.m_hr;
    }

    operationResult->GetStatus(&hr);
    if (SUCCEEDED(hr))
    {
        return operationResult->GetResult((IDxcBlob**)ppCode);
    }
    else
    {
        if (ppErrorMsgs)
            operationResult->GetErrorBuffer((IDxcBlobEncoding**)ppErrorMsgs);
        return hr;
    }
}

HRESULT WINAPI BridgeD3DCompile(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
    const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1,
    UINT Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs)
{
    CComPtr<IDxcLibrary> library;
    CComPtr<IDxcBlobEncoding> source;
    CComPtr<IDxcIncludeHandler> includeHandler;

    *ppCode = nullptr;
    if (ppErrorMsgs != nullptr)
        *ppErrorMsgs = nullptr;

    IFR(CreateLibrary(&library));
    IFR(library->CreateBlobWithEncodingFromPinned((LPBYTE)pSrcData, (UINT32)SrcDataSize, CP_ACP, &source));

    // Until we actually wrap the include handler, fail if there's a user-supplied handler.
    if (D3D_COMPILE_STANDARD_FILE_INCLUDE == pInclude)
    {
        IFT(library->CreateIncludeHandler(&includeHandler));
    }
    else if (pInclude)
    {
        return E_INVALIDARG;
    }

    try
    {
        CA2W pFileName(pSourceName);
        return CompileFromBlob(
            source, pFileName, pDefines, includeHandler, pEntrypoint, pTarget, Flags1, Flags2, ppCode, ppErrorMsgs);
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    catch (const CAtlException& err)
    {
        return err.m_hr;
    }
}

HRESULT WINAPI BridgeD3DCompile2(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
    const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1,
    UINT Flags2, UINT SecondaryDataFlags, LPCVOID pSecondaryData, SIZE_T SecondaryDataSize, ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs)
{
    if (SecondaryDataFlags == 0 || pSecondaryData == nullptr)
    {
        return BridgeD3DCompile(pSrcData, SrcDataSize, pSourceName, pDefines, pInclude, pEntrypoint, pTarget, Flags1,
            Flags2, ppCode, ppErrorMsgs);
    }
    return E_NOTIMPL;
}

HRESULT WINAPI BridgeD3DCompileFromFile(LPCWSTR pFileName, const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude,
    LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs)
{
    CComPtr<IDxcLibrary> library;
    CComPtr<IDxcBlobEncoding> source;
    CComPtr<IDxcIncludeHandler> includeHandler;
    HRESULT hr;

    *ppCode = nullptr;
    if (ppErrorMsgs != nullptr)
        *ppErrorMsgs = nullptr;

    hr = CreateLibrary(&library);
    if (FAILED(hr))
        return hr;
    hr = library->CreateBlobFromFile(pFileName, nullptr, &source);
    if (FAILED(hr))
        return hr;

    // Until we actually wrap the include handler, fail if there's a user-supplied handler.
    if (D3D_COMPILE_STANDARD_FILE_INCLUDE == pInclude)
    {
        IFT(library->CreateIncludeHandler(&includeHandler));
    }
    else if (pInclude)
    {
        return E_INVALIDARG;
    }

    return CompileFromBlob(
        source, pFileName, pDefines, includeHandler, pEntrypoint, pTarget, Flags1, Flags2, ppCode, ppErrorMsgs);
}

HRESULT WINAPI BridgeD3DDisassemble(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData, _In_ SIZE_T SrcDataSize,
    _In_ UINT Flags, _In_opt_ LPCSTR szComments, _Out_ ID3DBlob** ppDisassembly)
{
    CComPtr<IDxcLibrary> library;
    CComPtr<IDxcCompiler> compiler;
    CComPtr<IDxcBlobEncoding> source;
    CComPtr<IDxcBlobEncoding> disassemblyText;

    *ppDisassembly = nullptr;

    UNREFERENCED_PARAMETER(szComments);
    UNREFERENCED_PARAMETER(Flags);

    IFR(CreateLibrary(&library));
    IFR(library->CreateBlobWithEncodingFromPinned((LPBYTE)pSrcData, (UINT32)SrcDataSize, CP_ACP, &source));
    IFR(CreateCompiler(&compiler));
    IFR(compiler->Disassemble(source, &disassemblyText));
    IFR(disassemblyText.QueryInterface(ppDisassembly));

    return S_OK;
}

HRESULT WINAPI BridgeD3DReflect(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData, _In_ SIZE_T SrcDataSize,
    _In_ REFIID pInterface, _Out_ void** ppReflector)
{
    CComPtr<IDxcLibrary> library;
    CComPtr<IDxcBlobEncoding> source;
    CComPtr<IDxcContainerReflection> reflection;
    UINT shaderIdx;

    *ppReflector = nullptr;

    IFR(CreateLibrary(&library));
    IFR(library->CreateBlobWithEncodingOnHeapCopy((LPBYTE)pSrcData, (UINT32)SrcDataSize, CP_ACP, &source));
    IFR(CreateContainerReflection(&reflection));
    IFR(reflection->Load(source));
    IFR(reflection->FindFirstPartKind(hlsl::DFCC_DXIL, &shaderIdx));
    IFR(reflection->GetPartReflection(shaderIdx, pInterface, (void**)ppReflector));
    return S_OK;
}

HRESULT WINAPI BridgeReadFileToBlob(_In_ LPCWSTR pFileName, _Out_ ID3DBlob** ppContents)
{
    if (!ppContents)
        return E_INVALIDARG;
    *ppContents = nullptr;

    CComPtr<IDxcLibrary> library;
    IFR(CreateLibrary(&library));
    IFR(library->CreateBlobFromFile(pFileName, CP_ACP, (IDxcBlobEncoding**)ppContents));

    return S_OK;
}

HRESULT WINAPI D3DReflectDXILorDXBC(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData, _In_ SIZE_T SrcDataSize,
    _In_ REFIID pInterface, _Out_ void** ppReflector)
{
    return BridgeD3DReflect(pSrcData, SrcDataSize, IID_ID3D12ShaderReflection, ppReflector);
}
#else
HRESULT WINAPI D3DReflectDXILorDXBC(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData, _In_ SIZE_T SrcDataSize,
    _In_ REFIID pInterface, _Out_ void** ppReflector)
{
    return D3DReflect(pSrcData, SrcDataSize, IID_ID3D12ShaderReflection, ppReflector);
}
#endif
