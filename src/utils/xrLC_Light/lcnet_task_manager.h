#ifndef _LCNET_TASK_MANAGER_H_
#define _LCNET_TASK_MANAGER_H_
#include "hxgrid/Interface/IAgent.h"
#include "hxgrid/Interface/hxgridinterface.h"
#include "lightstab_interface.h"
#include "xrCore/Threading/Lock.hpp"

namespace lc_net
{
#include "hxgrid/Interface/IAgent.h"
// interface IGenericStream;

class net_execution;
class exec_pool;

class XRLC_LIGHT_API task_manager : public net_task_interface
{
    friend void Finalize(IGenericStream* inStream);
    static const u8 num_pools = 255;
    exec_pool* pools[num_pools];
    u8 current_pool;

    u32 start;

    CTimer start_time;
    IGridUser* _user;
    u32 session_id;
    u32 tasks_completed;
    bool _release;
    Lock pool_lock;
    Lock log_lock;
    Lock init_lock;

private:
    void send_task(IGridUser& user, u32 id);
    void receive_result(IGenericStream* inStream);
    bool initialize_session(u32 _session_id);

    void send_result(u8 pool_id, IGenericStream* outStream, net_execution& e);
    net_execution* receive_task(u8& pool_id, IAgent* agent, u32 sessionId, IGenericStream* inStream);
    virtual bool run_task(IAgent* agent, u32 sessionId, IGenericStream* inStream, IGenericStream* outStream);
    xr_vector<net_execution*>::iterator find(u32 id);

public:
    task_manager();
    exec_pool* run(LPCSTR name_pool);
    void add_task(net_execution* task);
    void startup();
    void progress(u32 task);

private:
    void release_user();
    void create_user();
    void user_init_thread();
    // static	void					release_user_thread_proc( void *_this );
    static void user_thread_proc(void* _this);

public:
    void wait_all();
    void release();
};

XRLC_LIGHT_API task_manager& get_task_manager();
};
#endif
