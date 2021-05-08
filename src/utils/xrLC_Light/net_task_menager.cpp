#include "stdafx.h"

#include "net_task_menager.h"

#include "hxgrid/Interface/IGenericStream.h"

#include "net_stream.h"
#include "xrLC_GlobalData.h"
#include "xrDeflector.h"

net_task_menager* g_net_task_menager = 0;

net_task_menager* get_net_task_menager() { return g_net_task_menager; }
void create_net_task_menager() { g_net_task_menager = xr_new<net_task_menager>(); }
void destroy_net_task_menager() { xr_delete(g_net_task_menager); }
net_task_menager::net_task_menager() : thProgress(0) {}
void __cdecl Finalize(IGenericStream* outStream)
{
    VERIFY(g_net_task_menager);
    INetReader r(outStream);
    get_net_task_menager()->receive(r);
    // VERIFY(outStream->GetLength()==4+9);
    // VERIFY(outStream->GetPos()==0);

    // DWORD n;
    // outStream->Read(&n,4);

    // outStream->Read(&pival[n-1],9);
}

static xrCriticalSection send_receive_data_lock;

static INetWriter* gl_data_write = 0;
void create_global_data_write()
{
    clMsg("create_global_data_write:  start");

    gl_data_write = xr_new<INetWriter>((IGenericStream*)(0), u32(-1));
    // INetWriter w( *stream, u32(-1) );
    // send_receive_data_lock.Enter();
    inlc_global_data()->write(*gl_data_write);
    // send_receive_data_lock.Leave();
    clMsg("create_global_data_write:  end, size %d", gl_data_write->count());
}

void __cdecl GetDataCallback(const char* dataDesc, IGenericStream** stream)
{
    clMsg("GetDataCallback: send start");
    VERIFY(xr_strcmp(dataDesc, "global_data") == 0);
    CTimer time;
    time.Start();
    R_ASSERT(gl_data_write);
    *stream = CreateGenericStream();
    //*stream = gl_data_write->net_stream();
    gl_data_write->send_not_clear(*stream);
    //*stream->Release();
    clMsg("GetDataCallback: send end time elapsed sec: %f, ", time.GetElapsed_sec());
    //(*stream) = new TGenericStream(20000000);

    //(*stream)->Write(globalDataStream->GetBasePointer(), globalDataStream->GetLength());
}

void net_task_menager::run()
{
    start_time.Start();
    create_global_data_write();
    inlc_global_data()->create_read_faces();

    IGridUser* user = CreateGridUserObject(IGridUser::VERSION);
    VERIFY(user);
    user->BindGetDataCallback(GetDataCallback);
    Status("Lighting...");

    VERIFY(inlc_global_data());
    u32 size = inlc_global_data()->g_deflectors().size();

    std::random_shuffle(inlc_global_data()->g_deflectors().begin(), inlc_global_data()->g_deflectors().end());
    for (u32 dit = 0; dit < size; dit++)
        pool.push_back(dit);

    FPU::m64r();
    Memory.mem_compact();

    for (u32 dit = 0; dit < size; dit++)
        send(*user, dit);

    user->WaitForCompletion();

    gl_data_write->clear();
    xr_delete(gl_data_write);
    inlc_global_data()->destroy_read_faces();

    user->Release();
    clMsg("%f net lightmaps seconds", start_time.GetElapsed_sec());
}

#ifdef _DEBUG
LPCSTR libraries =
    "XRLC_LightStab.dll,XRLC_Light.dll,xrCore.dll,xrCDB.dll,DXT.dll,BugTrapD.dll,msvcr80.dll,Microsoft.VC80.CRT."
    "manifest";
#else
LPCSTR libraries =
    "XRLC_LightStab.dll,XRLC_Light.dll,xrCore.dll,xrCDB.dll,DXT.dll,BugTrap.dll,msvcr80.dll,Microsoft.VC80.CRT."
    "manifest";
#endif

void net_task_menager::send(IGridUser& user, u32 id)
{
    // send_receive_data_lock.Enter();
    IGenericStream* stream = CreateGenericStream();
    {
        INetWriter w(stream, 100);
        w.w_u32(id);
    }
    u32 t_id = id;

    user.RunTask(libraries, "RunTask", stream, Finalize, &t_id, true);

    clMsg("send task : %d", id);
    DumpDeflctor(id);
    // send_receive_data_lock.Leave();
}
void net_task_menager::receive(INetReader& r)
{
    send_receive_data_lock.Enter();
    u32 id = r.r_u32();
    xr_vector<u32>::iterator it = std::find(pool.begin(), pool.end(), id);
    if (it == pool.end())
        return;
    pool.erase(it);
    u32 pool_size = pool.size();
    send_receive_data_lock.Leave();

    VERIFY(inlc_global_data());
    // inlc_global_data()->create_read_faces();
    inlc_global_data()->g_deflectors()[id]->read(r);
    // inlc_global_data()->destroy_read_faces();

    u32 size = inlc_global_data()->g_deflectors().size();

    clMsg("received task : %d", id);
    DumpDeflctor(id);

    VERIFY(size > 0);
    // thProgress+=(1.f/size);
    Progress(1.f - float(pool.size()) / float(size));
    clMsg("num task complited : %d , num task left %d  (task num %d)", size - pool_size, pool_size, size);
    if (pool.empty())
    {
        clMsg("calculation complited");
        clMsg("%f net lightmaps calculation seconds", start_time.GetElapsed_sec());
    }
}
