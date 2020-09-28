#ifndef _EXEC_POOL_H_
#define _EXEC_POOL_H_
#include "hxgrid/Interface/IAgent.h"
#include "hxgrid/Interface/hxgridinterface.h"
#include "xrCore/Threading/Lock.hpp"

namespace lc_net
{
class net_execution;
class task_manager;
class exec_pool
{
    string32 _name;
    u32 _start;
    u32 _end;
    u32 tasks_completed;
    Lock send_receive_lock;
    Lock run_lock;
    xr_vector<net_execution*> pool;
    CTimer start_time;
    task_manager& _task_manager;
    // IGridUser					*_user;
    bool _running;

public:
    ~exec_pool() { R_ASSERT(!_running); }
    exec_pool(task_manager* tm)
        : _task_manager(*tm), _running(false), tasks_completed(0), _start(u32(-1)), _end(u32(-1))
    {
        R_ASSERT(tm);
        xr_strcpy(_name, "net noname task");
    };
    exec_pool(u32 start, task_manager* tm)
        : _task_manager(*tm), _running(false), tasks_completed(0), _start(start), _end(start)
    {
        R_ASSERT(tm);
    };
    bool has(u32 id);
    u32 end() { return _end; }
    void add_task(net_execution* task);
    void wait();
    void set_name(LPCSTR name) { xr_strcpy(_name, name); }
    bool is_running();
    exec_pool& run(IGridUser& user, u8 pool_id);

    void send_task(IGridUser& user, IGenericStream* outStream, u8 pool_id, u32 id);
    void receive_result(IGenericStream* inStream);
    void remove_task(net_execution* e);
    void send_result(IGenericStream* outStream, net_execution& e);
    net_execution* receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream);

private:
};
}
#endif
