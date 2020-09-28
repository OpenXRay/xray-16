#include "stdafx.h"
#include "net_execution_factory.h"

#include "net_all_executions.h"

#include "net_execution_globals.h"
#include "xrCore/xrPool.h"
namespace lc_net
{
template <execution_types etype>
class tnet_execution : public tnet_execution_base<etype>
{
private:
    typedef tnet_execution<etype> self_type;
    net_execution_impl execution_impl;

public:
    tnet_execution(u32 id) : tnet_execution_base<etype>(id) { on_construct(); }
    explicit tnet_execution() : tnet_execution_base<etype>(u32(-1)) { on_construct(); }
private:
    void on_construct()
    {
        const xr_vector<e_net_globals>& v = exe_gl_reg().get_globals(etype);
        u32 size = v.size();
        for (u32 i = 0; i < size; ++i)
            globals().get(v[i]).add_ref();
    }
    virtual net_execution_impl& implementation() { return execution_impl; };
    virtual void send_task(IGridUser& user, IGenericStream* outStream, u32 id)
    {
        const xr_vector<e_net_globals>& v = exe_gl_reg().get_globals(etype);
        u32 size = v.size();
        for (u32 i = 0; i < size; ++i)
            globals().get(v[i]).on_task_send(outStream);
        tnet_execution_base<etype>::send_task(user, outStream, id);
        execution_impl.send_task(outStream);
    };

    virtual void receive_result(IGenericStream* outStream) { execution_impl.receive_result(outStream); };
    virtual bool receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream)
    {
        const xr_vector<e_net_globals>& v = exe_gl_reg().get_globals(etype);
        u32 size = v.size();
        for (u32 i = 0; i < size; ++i)
            if (!globals().get(v[i]).on_task_receive(agent, sessionId, inStream))
                return false;
        return execution_impl.receive_task(agent, sessionId, inStream);
    };
    virtual void send_result(IGenericStream* outStream) { execution_impl.send_result(outStream); };
    virtual bool execute(IAgent* agent, u32 sessionId)
    {
        net_task_callback callback(agent, sessionId);
        return execution_impl.execute(callback) && !callback.break_all();
    };
    virtual LPCSTR data_files(string_path& buf)
    {
        const xr_vector<e_net_globals>& v = exe_gl_reg().get_globals(etype);
        // xr_vector<e_net_globals>::const_iterator i = v.begin(), e = v.end();
        u32 size = v.size();
        buf[0] = 0;
        for (u32 i = 0; i < size; ++i)
        {
            string_path lbuf;
            strconcat(sizeof(string_path), buf, buf, globals().get(v[i]).files(lbuf));
        }
        return buf;
    }

public:
    virtual ~tnet_execution()
    {
        const xr_vector<e_net_globals>& v = exe_gl_reg().get_globals(etype);
        u32 size = v.size();
        for (u32 i = 0; i < size; ++i)
            globals().get(v[i]).free_ref();
    }
};

template <typename execution>
class execution_type_creator : public base_execution_type_creator
{
    poolSS<execution, 8 * 1024> pool;

    virtual void set_pool_size(u32 size){};
    virtual void free_pool() { pool.clear(); }
    virtual net_execution* create(u32 _net_id) { return xr_new<execution>(_net_id); }
    virtual net_execution* pool_create()
    {
        return xr_new<execution>(u32(-1));
        // return pool.create() ;
        return pool.create(); // spool<execution>::pool.create() ;
    }
    virtual void pool_destroy(net_execution*& e)
    {
        net_execution* _e = e;

        execution* ex = static_cast<execution*>(_e);
        VERIFY(ex == dynamic_cast<execution*>(_e));
        pool.destroy(ex);
        e = 0;
    }
    virtual u32 type() { return execution::class_type; }
};

template <typename execution>
static void register_type()
{
    execution_factory.register_type(xr_new<execution_type_creator<execution>>());
}

template <execution_types i>
struct it
{
    static const execution_types et = (execution_types)(i);
    static const execution_types next_et = (execution_types)(i + 1);
    typedef it<next_et> next;
    next ni;
    it() { register_type<tnet_execution<et>>(); }
};
template <>
struct it<et_last>
{
};

void factory::register_all()
{
    vec_types.resize(et_last, 0);
    it<et_lightmaps> i;
}
};
