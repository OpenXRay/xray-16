#include "StdAfx.h"
#include "RegistryFuncs.h"
#include "xrGameSpy/xrGameSpy_MainDefs.h"

#define REGISTRY_BASE HKEY_LOCAL_MACHINE

bool ReadRegistryValue(LPCSTR rKeyName, DWORD rKeyType, void* value)
{
#ifdef XR_PLATFORM_LINUX // FIXME!!!
    return true;
#else
    HKEY hKey = 0;
    long res = RegOpenKeyEx(REGISTRY_BASE, REGISTRY_PATH, 0, KEY_READ, &hKey);

    if (res != ERROR_SUCCESS)
    {
        Msg("! Unable to find %s in registry", REGISTRY_PATH);
        return false;
    }

    if (!hKey)
    {
        Msg("! Unable to find %s entry in registry", REGISTRY_PATH);
        return false;
    }

    string64 rBuf;
    DWORD KeyValueSize = 0;
    switch (rKeyType)
    {
    case REG_DWORD: { KeyValueSize = 4;
    }
    break;
    case REG_SZ: { KeyValueSize = 64;
    }
    break;
    default:
    {
        Msg("! Unknown registry data type.");
        return false;
    }
    break;
    };

    res = RegQueryValueEx(hKey, rKeyName, NULL, &rKeyType, (LPBYTE)rBuf, &KeyValueSize);
    if (hKey != 0)
        RegCloseKey(hKey);

    if (res != ERROR_SUCCESS)
    {
        Msg("! Unable to find %s entry in registry", rKeyName);
        return false;
    }

    memcpy(value, rBuf, KeyValueSize);
    return true;
#endif
};

bool WriteRegistryValue(LPCSTR rKeyName, DWORD rKeyType, const void* value)
{
#ifdef XR_PLATFORM_LINUX // FIXME!!!
    return true;
#else
    HKEY hKey = nullptr;

    long res = RegOpenKeyEx(REGISTRY_BASE, REGISTRY_PATH, 0, KEY_WRITE, &hKey);

    if (res != ERROR_SUCCESS)
    {
        Msg("! Unable to find %s in registry", REGISTRY_PATH);
        return false;
    }

    if (!hKey)
    {
        Msg("! Unable to find %s entry in registry", REGISTRY_PATH);
        return false;
    }

    u32 KeyValueSize = 0;
    switch (rKeyType)
    {
    case REG_DWORD: { KeyValueSize = 4;
    }
    break;
    case REG_SZ: { KeyValueSize = 64;
    }
    break;
    default:
    {
        Msg("! Unknown registry data type.");
        RegCloseKey(hKey);
        return false;
    }
    break;
    };

    res = RegSetValueEx(hKey, rKeyName, NULL, rKeyType, (LPBYTE)value, KeyValueSize);

    RegCloseKey(hKey);
    return true;
#endif
};

bool ReadRegistry_StrValue(LPCSTR rKeyName, char* value) 
{
#ifdef XR_PLATFORM_LINUX // FIXME!!!
    return true;
#else
    return ReadRegistryValue(rKeyName, REG_SZ, value);
#endif
}
void WriteRegistry_StrValue(LPCSTR rKeyName, const char* value)
{
#ifndef XR_PLATFORM_LINUX // FIXME!!!
    WriteRegistryValue(rKeyName, REG_SZ, value);
#endif
}
void ReadRegistry_DWValue(LPCSTR rKeyName, DWORD& value)
{
#ifndef XR_PLATFORM_LINUX // FIXME!!!
    ReadRegistryValue(rKeyName, REG_DWORD, &value);
#endif
}
void WriteRegistry_DWValue(LPCSTR rKeyName, const DWORD& value) 
{
#ifndef XR_PLATFORM_LINUX // FIXME!!
    WriteRegistryValue(rKeyName, REG_DWORD, &value);
#endif
}
u32 const ReadRegistry_BinaryValue(LPCSTR rKeyName, u8* buffer_dest, u32 const buffer_size)
{
#ifdef XR_PLATFORM_LINUX // FIXME!!!
    return u32(0);
#else
    HKEY hKey = 0;
    long res = RegOpenKeyEx(REGISTRY_BASE, REGISTRY_PATH, 0, KEY_READ, &hKey);

    if (res != ERROR_SUCCESS)
    {
        Msg("! Unable to find %s in registry", REGISTRY_PATH);
        return 0;
    }
    if (!hKey)
    {
        Msg("! Unable to find %s entry in registry", REGISTRY_PATH);
        return 0;
    }

    DWORD value_type = REG_BINARY;
    DWORD tmp_buffer_size = buffer_size;

    res = RegQueryValueEx(hKey, rKeyName, NULL, &value_type, buffer_dest, &tmp_buffer_size);

    if (res != ERROR_SUCCESS)
    {
        Msg("! Unable to find %s entry in registry", rKeyName);
        return 0;
    }

    return static_cast<u32>(tmp_buffer_size);
#endif
}

void WriteRegistry_BinaryValue(LPCSTR rKeyName, u8 const* buffer_src, u32 const buffer_size)
{
#ifndef XR_PLATFORM_LINUX // FIXME!!!
    HKEY hKey;

    long res = RegOpenKeyEx(REGISTRY_BASE, REGISTRY_PATH, 0, KEY_WRITE, &hKey);

    if (res != ERROR_SUCCESS)
    {
        Msg("! Unable to find %s in registry", REGISTRY_PATH);
        return;
    }

    if (!hKey)
    {
        Msg("! Unable to find %s entry in registry", REGISTRY_PATH);
        return;
    }

    res = RegSetValueEx(hKey, rKeyName, NULL, REG_BINARY, buffer_src, buffer_size);

    RegCloseKey(hKey);
#endif
}
