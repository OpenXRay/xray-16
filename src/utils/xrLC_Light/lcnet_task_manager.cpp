#include "stdafx.h"
#include "lcnet_task_manager.h"
#include "net_execution.h"
#include "xrlc_globaldata.h"
#include "file_compress.h"
#include "serialize.h"
#include "net_execution_factory.h"
#include "net_global_data_cleanup.h"

#include "net_exec_pool.h"

namespace lc_net
{
task_manager g_task_manager;

task_manager& get_task_manager() { return g_task_manager; }
XRLC_LIGHT_API net_task_interface* g_net_task_interface = &g_task_manager;

void __cdecl Finalize(IGenericStream* inStream)
{
    get_task_manager().receive_result(inStream);
    // inStream->Clear();
}

task_manager::task_manager()
    : _user(0), tasks_completed(0), current_pool(0), start(0), session_id(u32(-1)), _release(false)
{
    for (u8 i = 0; i < num_pools; ++i)
        pools[i] = 0;
    // create_global_data_write("");
}

bool task_manager::initialize_session(u32 _session_id)
{
    init_lock.Enter();
    bool ret = false;
    if (session_id == u32(-1))
    {
        session_id = _session_id;
        ret = true;
    }
    else
        ret = (session_id == _session_id);
    init_lock.Leave();
    return ret;
}
void task_manager::receive_result(IGenericStream* inStream)
{
    u8 pool_id(u8(-1));
    // u32 task_id ( u32(-1) ), type_id ( u32(-1)  );

    read_task_pool(inStream, pool_id);
    pools[pool_id]->receive_result(inStream);
}

void task_manager::send_task(IGridUser& user, u32 id) {}
void task_manager::send_result(u8 pool_id, IGenericStream* outStream, net_execution& e)
{
    write_task_pool(outStream, pool_id);
    pools[pool_id]->send_result(outStream, e);
}

net_execution* task_manager::receive_task(u8& pool_id, IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    // u8 pool_id  ( u8(-1) );
    read_task_pool(inStream, pool_id);
    R_ASSERT(pool_id >= 0);
    R_ASSERT(pool_id < num_pools);

    pool_lock.Enter();
    if (pools[pool_id] == 0)
        pools[pool_id] = xr_new<exec_pool>(this);
    pool_lock.Leave();
    return pools[pool_id]->receive_task(agent, sessionId, inStream);
}

void __cdecl data_cleanup_callback(const char* dataDesc, IGenericStream** stream);
void task_manager::startup()
{
    start_time.Start();
    tasks_completed = 0;
    // create_user( );
    Threading::SpawnThread(task_manager::user_thread_proc, "release-user", 1024 * 1024, this);
    for (;;)
    {
        Sleep(1);
        bool user_inited = false;
        init_lock.Enter();
        user_inited = !!_user;
        init_lock.Leave();
        if (user_inited)
            break;
    }

    R_ASSERT(_user);
    FPU::m64r();
    Memory.mem_compact();
}
void task_manager::create_user()
{
    init_lock.Enter();
    R_ASSERT(!_user);
    R_ASSERT(!_release);
    _user = CreateGridUserObject(IGridUser::VERSION);
    VERIFY(_user);
    _user->BindGetDataCallback(data_cleanup_callback);
    init_lock.Leave();
}
void task_manager::user_init_thread()
{
    create_user();
    for (;;)
    {
        bool release = false;
        Sleep(1000);
        init_lock.Enter();
        release = _release;
        init_lock.Leave();
        if (release)
            break;
    }
    release_user();
}
void task_manager::wait_all()
{
    for (;;)
    {
        Sleep(1000);
        u32 num_running = 0;
        for (u8 i = 0; i < num_pools; ++i)
            if (pools[i] && pools[i]->is_running())
                ++num_running;
        if (num_running == 0)
            break;
    }
    // R_ASSERT(_user);
    //_user->WaitForCompletion();
    // release();
}
exec_pool* task_manager::run(LPCSTR name_pool)
{
    pool_lock.Enter();

    if (!pools[current_pool])
    {
        pool_lock.Leave();
        return 0;
    }
    pools[current_pool]->set_name(name_pool);
    start = pools[current_pool]->end();
    u8 lrun = current_pool;
    ++current_pool;
    R_ASSERT(current_pool < num_pools);

    pool_lock.Leave();

    R_ASSERT(_user);
    pools[lrun]->run(*_user, lrun);
    return pools[lrun];
}
void task_manager::progress(u32 task)
{
    u32 l_completed = 0;
    log_lock.Enter();
    ++tasks_completed;
    l_completed = tasks_completed;
    log_lock.Leave();
    Logger.Progress(float(l_completed) / float(start));
}

//	void task_manager::release_user_thread_proc(void *_this )
//	{
//		((task_manager*)_this)->release_user();
//	}
void task_manager::user_thread_proc(void* _this) { ((task_manager*)_this)->user_init_thread(); }
void task_manager::release_user()
{
    init_lock.Enter();
    if (!_user)
    {
        init_lock.Leave();
        return;
    }
    R_ASSERT(_user);
    //_user->CancelTasks();
    //_user->Release();
    _user = 0;
    for (u8 i = 0; i < num_pools; ++i)
        xr_delete(pools[i]);
    init_lock.Leave();
}

void task_manager::release()
{
    for (u8 i = 0; i < num_pools; ++i)
        R_ASSERT(!(pools[i]) || !(pools[i]->is_running()));
    init_lock.Enter();
    _release = true;
    init_lock.Leave();
    // Threading::SpawnThread(task_manager::release_user_thread_proc, "release-user", 1024 * 1024, this);
}

void task_manager::add_task(net_execution* task)
{
    pool_lock.Enter();

    if (!pools[current_pool])
        pools[current_pool] = xr_new<exec_pool>(start, this);

    pools[current_pool]->add_task(task);

    pool_lock.Leave();
}
};
