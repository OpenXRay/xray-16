#include "stdafx.h"

#include "net_task_manager.h"

#include "hxgrid/Interface/IGenericStream.h"

#include "net_stream.h"
#include "xrLC_GlobalData.h"
#include "xrDeflector.h"

void compress(LPCSTR f_in_out);

#ifdef NET_CMP
static xr_vector<std::pair<CDeflector*, CDeflector*>> diff;
void DumpDiff()
{
    Msg("diference: %d ", diff.size());
    xr_vector<std::pair<CDeflector *, CDeflector *>>::iterator i = diff.begin(), b = diff.begin(), e = diff.end();
    for (; i != e; ++i)
    {
        Msg("diff %d", u32(i - b));
        DumpDeflctor(*(*i).first);
        DumpDeflctor(*(*i).second);
    }
}
#endif

net_task_manager* g_net_task_manager = 0;

net_task_manager* get_net_task_manager() { return g_net_task_manager; }
void create_net_task_manager() { g_net_task_manager = xr_new<net_task_manager>(); }
void destroy_net_task_manager() { xr_delete(g_net_task_manager); }
net_task_manager::net_task_manager() : thProgress(0) {}
void __cdecl Finalize(IGenericStream* outStream)
{
    VERIFY(g_net_task_manager);
    INetBlockReader r(outStream);
    // INetReaderGenStream r(outStream);
    get_net_task_manager()->receive(r);
    // VERIFY(outStream->GetLength()==4+9);
    // VERIFY(outStream->GetPos()==0);

    // DWORD n;
    // outStream->Read(&n,4);

    // outStream->Read(&pival[n-1],9);
}

static Lock send_receive_data_lock;
class INetFileBuffWriter;
static INetFileBuffWriter* gl_data_write = 0;
static CVirtualFileRW* g_net_data = 0;

void net_task_manager::create_global_data_write(LPCSTR save_path)
{
    FPU::m64r();
    Memory.mem_compact();

    clMsg("create_global_data_write:  start");
#ifdef NET_CMP
    string_path fn;
    strconcat(sizeof(fn), fn, save_path, "cl_global_data");
    INetReader fr(0);
    fr.load_buffer(fn);
    inlc_global_data()->clear();
    inlc_global_data()->read(fr);
#endif
#if !defined(NET_CMP) && !defined(LOAD_GL_DATA)
    std::random_shuffle(inlc_global_data()->g_deflectors().begin(), inlc_global_data()->g_deflectors().end());
#endif

#ifndef LOAD_GL_DATA
    // gl_data_write = new INetFileBuffWriter( "tmp_global_data", 1024*1024/2,false);
    // inlc_global_data()->write( *gl_data_write );
    // gl_data_write->w_close();

    string_path global_data_file_name;

    FS.update_path(global_data_file_name, "$app_root$", gl_data_net_file_name);
    IWriter* file = FS.w_open(global_data_file_name);
    inlc_global_data()->write(*file);
    FS.w_close(file);
    compress(global_data_file_name);
    // string_path bin_path;
    // FS.update_path( bin_path,"$app_root$", gl_data_net_file_name );

    // g_net_data = new CVirtualFileRW(global_data_file_name);

    // dbg_buf = xr_malloc( 560000000, "dbg_buf" );
    ////////////////
    /*{
        string_path			 blfile_name;
        FS.update_path		( blfile_name, "$level$", "btmp_global_data" );

        string_path			 blfile_name_z;
        FS.update_path		( blfile_name_z, "$level$", "btmp_global_data_z" );

        string_path			 blfile_name_uz;
        FS.update_path		( blfile_name_uz, "$level$", "btmp_global_data_uz" );
        compress( blfile_name, blfile_name_z );
        decompress( blfile_name_z, blfile_name_uz );
        //"btmp_global_data"
    }
    */
    ///////////////
    clMsg("create_global_data_write:  end");
#else
    // gl_data_write = new INetFileBuffWriter( "tmp_global_data", 1024*1024/2,true);
    // gl_data_write->w_close();
    string_path lfile_name;
    FS.update_path(lfile_name, "$level$", "tmp_global_data");
    g_net_data = xr_new<CVirtualFileRW>(lfile_name);
#endif

// send_receive_data_lock.Leave();

#ifdef NET_CMP
    gl_data_write->save_buffer(fn);
#endif
}

void __cdecl GetDataCallback(const char* dataDesc, IGenericStream** stream)
{
    clMsg("GetDataCallback: send start");
    VERIFY(xr_strcmp(dataDesc, "global_data") == 0);
    CTimer time;
    time.Start();
    // R_ASSERT(gl_data_write);

    *stream = xr_new<CGenStreamOnFile>(g_net_data); // CreateGenericStream();
    //*stream = gl_data_write->net_stream();

    // gl_data_write->send_not_clear(*stream);
    // xr_delete( *stream );
    clMsg("GetDataCallback: send end time elapsed sec: %f, ", time.GetElapsed_sec());
    //(*stream) = new TGenericStream(20000000);

    //(*stream)->Write(globalDataStream->GetBasePointer(), globalDataStream->GetLength());
}

void net_task_manager::run()
{
    start_time.Start();
#ifndef NET_CMP

    create_global_data_write("");
#endif
    // inlc_global_data()->create_read_faces();

    IGridUser* user = CreateGridUserObject(IGridUser::VERSION);
    VERIFY(user);
    // user->BindGetDataCallback(GetDataCallback);
    Status("Lighting...");

#if !defined(NET_CMP) && !defined(LOAD_GL_DATA)
    VERIFY(inlc_global_data());
    u32 size = inlc_global_data()->g_deflectors().size();
    for (u32 dit = 0; dit < size; dit++)
        pool.push_back(dit);
#else
    pool.push_back(0);
    pool.push_back(1);
#endif

    FPU::m64r();
    Memory.mem_compact();
#if !defined(NET_CMP) && !defined(LOAD_GL_DATA)
    for (u32 dit = 0; dit < size; dit++)
        send(*user, dit);
    user->WaitForCompletion();
    // gl_data_write->clear();
    // xr_delete(gl_data_write);
    xr_delete(g_net_data);
#else
    send(*user, 0);
    send(*user, 1);
    user->WaitForCompletion();
    xr_delete(g_net_data);
#endif

    // inlc_global_data()->destroy_read_faces();

    user->Release();
#ifdef NET_CMP
    DumpDiff();
#endif
    clMsg("%f net lightmaps seconds", start_time.GetElapsed_sec());
}

//#ifdef _DEBUG
// LPCSTR libraries =
// "XRLC_LightStab.dll,XRLC_Light.dll,xrCore.dll,xrCDB.dll,DXT.dll,BugTrapD.dll,msvcr80.dll,Microsoft.VC80.CRT.manifest,tmp_global_data0,tmp_global_data1,tmp_global_data2,tmp_global_data3,tmp_global_data4,tmp_global_data5";
//#else
// LPCSTR libraries =
// "XRLC_LightStab.dll,XRLC_Light.dll,xrCore.dll,xrCDB.dll,DXT.dll,BugTrap.dll,msvcr80.dll,Microsoft.VC80.CRT.manifest,tmp_global_data0,tmp_global_data1,tmp_global_data2,tmp_global_data3,tmp_global_data4,tmp_global_data5";
//#endif

void send_lightmap_task(IGridUser& user, u32 deflector_id) {}
void net_task_manager::send(IGridUser& user, u32 deflector_id)
{
    clMsg("send task : %d", deflector_id);
    // send_receive_data_lock.Enter();
    IGenericStream* stream = CreateGenericStream();
    {
        INetMemoryBuffWriter w(stream, 100);
        // INetIWriterGenStream w( stream, 100 );
        w.w_u32(deflector_id);
    }

    u32 t_id = deflector_id;
    string_path data;
    strconcat(sizeof(data), data, libraries, ",", gl_data_net_file_name);
    user.RunTask(data, "RunTask", stream, Finalize, &t_id, true);

#ifndef LOAD_GL_DATA
    DumpDeflctor(deflector_id);
#endif
    // send_receive_data_lock.Leave();
}
void net_task_manager::receive(INetReader& r)
{
#ifdef LOAD_GL_DATA
    return;
#endif
    send_receive_data_lock.Enter();
    u32 id = r.r_u32();
    xr_vector<u32>::iterator it = std::find(pool.begin(), pool.end(), id);
    if (it == pool.end())
    {
        send_receive_data_lock.Leave();
        //		CDeflector temp;
        //		temp.read( r );
        return;
    }
    pool.erase(it);
    u32 pool_size = pool.size();
    send_receive_data_lock.Leave();

    VERIFY(inlc_global_data());
// inlc_global_data()->create_read_faces();
#ifdef NET_CMP
    CDeflector* netD = xr_new<CDeflector>();
    CDeflector* D = inlc_global_data()->g_deflectors()[id];
    netD->read(r);
    if (!netD->similar(*D))
    {
        send_receive_data_lock.Enter();
        diff.push_back(std::pair<CDeflector*, CDeflector*>(D, netD));
        send_receive_data_lock.Leave();
    }
    else
        xr_delete(netD);
#else
    inlc_global_data()->g_deflectors()[id]->read(r);
#endif
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
