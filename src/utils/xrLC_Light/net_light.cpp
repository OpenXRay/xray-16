#include "stdafx.h"

#include "net_light.h"
#include "hxgrid/Interface/IAgent.h"
#include "hxgrid/Interface/IGenericStream.h"
#include "net_light_task.h"
#include "net_stream.h"
#include "xrlc_globaldata.h"
#include "xrdeflector.h"
#include "net_task.h"
//==============================================================================
// function RunTask(): boolean;
//==============================================================================

static Lock block;
LPCSTR dataDesc = "global_data";

xr_vector<u32> net_pool;
/*
static struct unload
{
    IAgent* agent;
    ~unload()
    {
        if(!agent)
            return;
        LPCRITICAL_SECTION gcs;
        agent->GetGlobalCriticalSection( &gcs );
        LeaveCriticalSection( gcs );
    }
} _unload;
*/
#pragma warning(push)
#pragma warning(disable : 4995)
u32 g_sessionId = u32(-1);

// bool  GetGlobalData( IAgent* agent,
//				    u32 sessionId );
bool TaskReceive(net_task& task, IAgent* agent, u32 sessionId, IGenericStream* inStream);

bool GetGlobalData(IAgent* agent, u32 sessionId)
{
    if (!inlc_global_data())
    {
        VERIFY(agent);
        net_pool.clear();

        /*
                  IGenericStream* globalDataStream=0;
                  HRESULT rz = agent->GetData(sessionId, dataDesc, &globalDataStream);

                  if (rz!=S_OK)
                          return false;
        */
        string_path cache_dir;
        HRESULT rz = agent->GetSessionCacheDirectory(sessionId, cache_dir);
        if (rz != S_OK)
            return false;
        strconcat(sizeof(cache_dir), cache_dir, cache_dir, gl_data_net_file_name);

        /*
                IWriter* file = FS.w_open( cache_dir );
                R_ASSERT(file);
                file->w( globalDataStream->GetCurPointer(), globalDataStream->GetLength() );

                free(globalDataStream->GetCurPointer());
                FS.w_close(file);

                agent->FreeCachedData(sessionId, dataDesc);
                ULONG r =globalDataStream->AddRef();
                r = globalDataStream->Release();
                if(r>0)
                       globalDataStream->Release();
                agent->FreeCachedData(sessionId, dataDesc);
                Memory.mem_compact	();
       */

        DataReadCreate(cache_dir);

        return true;
    }
    return true;
}

bool TaskReceive(net_task& task, IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    bool ret = false;
    __try
    {
        ret = GetGlobalData(agent, sessionId) && task.receive(inStream);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Msg("accept!");
        return ret;
    }
    FPU::m64r();
    return ret;
}

class net_task_interface_impl : public net_task_interface
{
    bool TaskReceive(net_task& task, IAgent* agent, u32 sessionId, IGenericStream* inStream)
    {
        return ::TaskReceive(task, agent, sessionId, inStream);
    }
    /*bool  GetGlobalData( IAgent* agent,
                       DWORD sessionId )
    {
           GetGlobalData()
    }*/
    bool TaskSendResult(net_task& task, IGenericStream* outStream)
    {
        bool ret = false;
        __try
        {
            ret = task.send(outStream);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Msg("accept!");
            return ret;
        }
        return ret;
    }

    bool RunTask(IAgent* agent, u32 sessionId, IGenericStream* inStream, IGenericStream* outStream)
    {
        block.Enter();

        g_sessionId = sessionId;
        net_task task(agent, sessionId);

        if (!TaskReceive(task, agent, sessionId, inStream))
        {
            block.Leave();
            return false;
        }
        block.Leave();

        task.run();

        block.Enter();

        if (!TaskSendResult(task, outStream))
        {
            block.Leave();
            return false;
        }
        block.Leave();
        return true;
    }
} g_net_task_interface_impl;
#pragma warning(pop)

XRLC_LIGHT_API net_task_interface* g_net_task_interface = &g_net_task_interface_impl;
/*
XRLC_LIGHT_API  void __cdecl  EndSession(IAgent* agent, DWORD sessionId)
{
    LPCRITICAL_SECTION gcs;
      agent->GetGlobalCriticalSection( &gcs );
      //LeaveCriticalSection( gcs );


*/
