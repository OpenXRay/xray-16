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
#ifndef __CCRYDX12OBJECT__
#define __CCRYDX12OBJECT__

#include "API/DX12.hpp"

#define DX12_BASE_OBJECT(DerivedType, InterfaceType) \
    typedef DerivedType Class;                       \
    typedef _smart_ptr<Class> Ptr;               \
    typedef _smart_ptr<const Class> ConstPtr;    \
    typedef InterfaceType Super;                     \
    typedef InterfaceType Interface;                 \

#define DX12_OBJECT(DerivedType, SuperType)       \
    typedef DerivedType Class;                    \
    typedef _smart_ptr<Class> Ptr;            \
    typedef _smart_ptr<const Class> ConstPtr; \
    typedef SuperType Super;                      \

#include "CryDX12Guid.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CCryDX12Buffer;
class CCryDX12DepthStencilView;
class CCryDX12Device;
class CCryDX12DeviceContext;
class CCryDX12MemoryManager;
class CCryDX12Query;
class CCryDX12RenderTargetView;
class CCryDX12SamplerState;
class CCryDX12Shader;
class CCryDX12ShaderResourceView;
class CCryDX12SwapChain;
class CCryDX12Texture1D;
class CCryDX12Texture2D;
class CCryDX12Texture3D;
class CCryDX12UnorderedAccessView;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class CCryDX12Object
    : public T
{
public:
    DX12_BASE_OBJECT(CCryDX12Object, T);

    CCryDX12Object()
        : m_RefCount(0)
    {
    }

    virtual ~CCryDX12Object()
    {
        DX12_LOG("CCryDX12 object destroyed: %p", this);
    }

    #pragma region /* IUnknown implementation */

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement((volatile LONG*)&m_RefCount);
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        ULONG RefCount;
        if (!(RefCount = InterlockedDecrement((volatile LONG*)&m_RefCount)))
        {
            delete this;
            return 0;
        }

        return RefCount;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) final
    {
        if ((riid == __uuidof(T)) || (riid == __uuidof(ID3D11Device)) || (riid == __uuidof(ID3D11DeviceContext)))
        {
            if (ppvObject)
            {
                *reinterpret_cast<T**>(ppvObject) = static_cast<T*>(this);
                static_cast<T*>(this)->AddRef();
            }

            return S_OK;
        }

        return E_NOINTERFACE;
    }

    #pragma endregion

private:
    int m_RefCount;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class CCryDX12GIObject
    : public T
{
public:
    DX12_BASE_OBJECT(CCryDX12GIObject, T);

    CCryDX12GIObject()
        : m_RefCount(0)
    {
    }

    virtual ~CCryDX12GIObject()
    {
    }

    #pragma region /* IUnknown implementation */

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement((volatile LONG*)&m_RefCount);
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        ULONG RefCount;
        if (!(RefCount = InterlockedDecrement((volatile LONG*)&m_RefCount)))
        {
            delete this;
            return 0;
        }

        return RefCount;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) final
    {
        if ((riid == __uuidof(T)) ||
            (riid == __uuidof(IDXGIDevice) && __uuidof(IDXGIDevice3) == __uuidof(T)) ||
            (riid == __uuidof(IDXGIFactory) && __uuidof(IDXGIFactory4) == __uuidof(T)) ||
            (riid == __uuidof(IDXGIAdapter) && __uuidof(IDXGIAdapter3) == __uuidof(T)) ||
            (riid == __uuidof(IDXGIOutput) && __uuidof(IDXGIOutput4) == __uuidof(T)) ||
            (riid == __uuidof(IDXGISwapChain) && __uuidof(IDXGISwapChain3) == __uuidof(T)))
        {
            if (ppvObject)
            {
                *reinterpret_cast<T**>(ppvObject) = static_cast<T*>(this);
                static_cast<T*>(this)->AddRef();
            }

            return S_OK;
        }

        return E_NOINTERFACE;
    }

    #pragma endregion

    #pragma region /* IDXGIObject implementation */

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        _In_  REFGUID Name,
        UINT DataSize,
        _In_reads_bytes_(DataSize) const void* pData)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        _In_  REFGUID Name,
        _In_  const IUnknown* pUnknown)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        _In_  REFGUID Name,
        _Inout_  UINT* pDataSize,
        _Out_writes_bytes_(*pDataSize) void* pData)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE GetParent(
        _In_  REFIID riid,
        _Out_ void** ppParent)
    {
        return -1;
    }

    #pragma endregion

private:
    int m_RefCount;
};

#endif // __CCRYDX12OBJECT__
