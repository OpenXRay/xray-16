#include "stdafx.h"
#include "net_exec_pool.h"

#include "net_execution.h"
#include "xrlc_globaldata.h"
#include "file_compress.h"
#include "serialize.h"
#include "net_execution_factory.h"
#include "net_global_data_cleanup.h"
#include "lcnet_task_manager.h"
#define LOG_ALL_NET_TASKS

LPCSTR make_time(string64& buf, float fsec)
{
    // char		buf[64];

    int sec = iFloor(fsec);
    if (sec < 0)
        sec = 0;
    xr_sprintf(buf, "%2.0d:%2.0d:%2.0d", sec / 3600, (sec % 3600) / 60, sec % 60);
    size_t len = xr_strlen(buf);
    for (size_t i = 0; i < len; i++)
        if (buf[i] == ' ')
            buf[i] = '0';
    return buf;
}

namespace lc_net
{
void exec_pool::add_task(net_execution* e)
{
    pool.push_back(e);
    ++_end;
}

bool exec_pool::has(u32 id)
{
    R_ASSERT(id != u32(-1));
    R_ASSERT(_start != u32(-1));
    R_ASSERT(_end != u32(-1));
    R_ASSERT(_start < _end);

    return id >= _start && id < _end;
}

void exec_pool::receive_result(IGenericStream* inStream)
{
    u32 id = u32(-1), type = u32(-1); // r.r_u32();
    read_task_caption(inStream, id, type);

    // xr_vector<u32>::iterator it =std::find( pool.begin(), pool.end(), id );
    const u32 size = pool.size();

    R_ASSERT(_start != u32(-1));
    R_ASSERT(_end != u32(-1));

    R_ASSERT(_end > 0);
    R_ASSERT(_start < _end);

    R_ASSERT(id >= _start);
    R_ASSERT(id < _end);

    R_ASSERT(_running);
    R_ASSERT(size > 0);
    R_ASSERT(id >= 0);

    u32 pos = id - _start;

    R_ASSERT(pos >= 0);
    R_ASSERT(pos < size);

    send_receive_lock.Enter();

    net_execution* e = pool[pos];
    R_ASSERT(e->type() == type);
    if (e == 0)
    {
        send_receive_lock.Leave();
        return;
    }
    pool[pos] = 0;
    ++tasks_completed;
    u32 l_completed = tasks_completed;
    send_receive_lock.Leave();
    e->receive_result(inStream);
    // xr_delete( e );
    execution_factory.destroy(e);
    _task_manager.progress(id);
#ifdef LOG_ALL_NET_TASKS
    Logger.clMsg("%s received task : %d", _name, id);

    // Progress( float( tasks_completed )/float( size ) );
    Logger.clMsg("num task complited : %d , num task left %d  (task num %d)", l_completed, size - l_completed, size);
#endif
    R_ASSERT(l_completed <= size);
    if (l_completed == size)
    {
        string64 buf;
        Logger.clLog(" %s, calculation complited", _name);
        // clMsg	("%f %s calculation seconds",start_time.GetElapsed_sec(), _name );

        Logger.clLog("%s %s calculation time", make_time(buf, start_time.GetElapsed_sec()), _name);
        // Status	("%s %s calculation time",make_time( buf,start_time.GetElapsed_sec() ), _name );

        // xr_sprintf( buf, "%s %s calculation time",make_time( buf,start_time.GetElapsed_sec() ), _name );
        // Phase( buf );
        //		}

        //		if( l_completed == size )
        //		{
        execution_factory.free_pool(type);
        run_lock.Enter();
        _running = false;
        run_lock.Leave();
    }
}

void exec_pool::wait()
{
    do
    {
        Sleep(1000);
    } while (is_running());
}
bool exec_pool::is_running()
{
    bool running = true;
    run_lock.Enter();
    running = _running;
    run_lock.Leave();
    return running;
}
exec_pool& exec_pool::run(IGridUser& user, u8 pool_id)
{
    start_time.Start();
    R_ASSERT(!_running);
    _running = true;
    u32 size = pool.size();

    // IGenericStream* stream  = CreateGenericStream();
    for (u32 dit = _start; dit < _end; dit++)
        send_task(user, 0, pool_id, dit);

    return *this;
}
void __cdecl Finalize(IGenericStream* outStream);
Lock run_task_lock;
void exec_pool::send_task(IGridUser& user, IGenericStream* Stream, u8 pool_id, u32 id)
{
    R_ASSERT(_running);
    R_ASSERT(has(id));
    IGenericStream* outStream = CreateGenericStream();
    //////////////////////////////////////////////////////
    write_task_pool(outStream, pool_id); ////////////////////
    //////////////////////////////////////////////////////
    cleanup().on_net_send(outStream);

    u32 pos = id - _start;
    VERIFY(pos < pool.size());
    net_execution* e = pool[pos];
    VERIFY(e != 0);
    write_task_caption(outStream, id, e->type());
#ifdef LOG_ALL_NET_TASKS
    Logger.clMsg(" %s, send task : %d", _name, id);
//
#endif

    e->send_task(user, outStream, id);

    DWORD t_id = id;
    string_path data;
    string_path files;
    strconcat(sizeof(data), data, libraries, e->data_files(files));
    run_task_lock.Enter();
    bool ok = false;
run_task:;

    __try
    {
        user.RunTask(data, "RunTask", outStream, Finalize, &t_id, true);

        ok = true;
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (!ok)
        {
            Msg("accept run task");
            goto run_task;
        }
    }

    run_task_lock.Leave();
    return;
}

net_execution* exec_pool::receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    __try
    {
        cleanup().on_net_receive(agent, sessionId, inStream);
        u32 id = u32(-1), type = u32(-1);
        read_task_caption(inStream, id, type);

        send_receive_lock.Enter();

        for (size_t i = 0; i < pool.size(); i++)
        {
            net_execution* it = pool[i];
            R_ASSERT(it);
            if (it->id() == id)
            {
                send_receive_lock.Leave();
                return 0;
            }
        }

        net_execution* e = execution_factory.create(type, id);
        pool.push_back(e);
        send_receive_lock.Leave();
        e->receive_task(agent, sessionId, inStream);

        return e;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Msg("accept!");
        return 0;
    }
}

void exec_pool::remove_task(net_execution* e)
{
    send_receive_lock.Enter();
    xr_vector<net_execution*>::iterator i = std::find(pool.begin(), pool.end(), e);
    R_ASSERT(i != pool.end());
    net_execution* pe = *i;
    R_ASSERT(pe == e);
    R_ASSERT(pe->id() == e->id());
    pool.erase(i);
    send_receive_lock.Leave();
    xr_delete(e);
}

void exec_pool::send_result(IGenericStream* outStream, net_execution& e)
{
    write_task_caption(outStream, e.id(), e.type());
    e.send_result(outStream);
}
}
