#pragma once

#include "NET_Shared.h"
#include "Common/Noncopyable.hpp"

class Lock;

struct SLogPacket
{
    u32 m_u32Time;
    u32 m_u32Size;
    u16 m_u16Type;
    string64 m_sTypeStr;
    bool m_bIsIn;
};

class INetLog : Noncopyable
{
    FILE* m_pLogFile;
    string1024 m_cFileName;
    u32 m_dwStartTime;

    Lock* m_pcs;

    xr_vector<SLogPacket> m_aLogPackets;

    void FlushLog();

public:
    INetLog(pcstr sFileName, u32 dwStartTime);
    ~INetLog();

    void LogPacket(u32 Time, NET_Packet* pPacket, bool IsIn = false);
    void LogData(u32 Time, void* data, u32 size, bool IsIn = false);
};

/*
// Singleton template definition
template <class T>
class CSingleton
{
    static T* _self;
    static int _refcount;

public:
    //whether singleton will delete itself on FreeInst
    //when _refcount = 0
    //otherwise user should call DestroySingleton() manually
    static bool _on_self_delete;

    CSingleton() {}
    virtual ~CSingleton() { _self = nullptr; }

    static void DestroySingleton()
    {
        if (!_self) return;
        Log("DestroySingleton::RefCounter:", _refcount);
        VERIFY(_on_self_delete == false);
        VERIFY(_refcount == 0);
        xr_delete(_self);
    };

    static T* Instance()
    {
        if (!_self) _self = xr_new<T>();
        ++_refcount;
        return _self;
    }

    void FreeInst()
    {
        if (0 == --_refcount)
        {
            if (_on_self_delete)
            {
                CSingleton<T>* ptr = this;
                xr_delete(ptr);
            }
        }
    }
};

template <class T>
T* CSingleton<T>::_self = nullptr;
template <class T>
int CSingleton<T>::_refcount = 0;
template <class T>
bool CSingleton<T>::_on_self_delete = true;
*/
