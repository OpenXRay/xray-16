#include "stdafx.h"

#include "net_global_data.h"
#include "net_all_globals.h"
#include "net_global_data_cleanup.h"
#include "net_execution_globals.h"
namespace lc_net
{
static net_globals globs;

net_globals& globals() { return globs; }
static LPCSTR global_data_file_path(LPCSTR name, string_path& path_name)
{
    FS.update_path(path_name, "$app_root$", name);
    return path_name;
}

bool global_data_file_path(LPCSTR name, IAgent* agent, u32 sessionId, string_path& path_name)
{
    HRESULT rz = agent->GetSessionCacheDirectory(sessionId, path_name);
    if (rz != S_OK)
        return false;
    strconcat(sizeof(path_name), path_name, path_name, name);
    return true;
}

template <e_net_globals gl_type>
class tnet_global_data : public tnet_global_data_base<gl_type>
// public net_global_data,
// public net_global_data_impl<gl_type>
{
    typedef net_global_data_impl<gl_type> impl;
    // net_global_data_impl<gl_type>	impl;

    Lock create_data_lock;
    Lock ref_lock;
    u32 _id;
    u32 _use_count;
    bool _clear;

public:
    tnet_global_data() : _id(0), _use_count(0), _clear(false) {}
    void clear()
    {
        VERIFY(_id != 0);
        ref_lock.Enter();
        create_data_lock.Enter();
        if (_use_count > 0)
            _clear = true;
        else
            destroy_data();
        create_data_lock.Leave();
        ref_lock.Leave();
    }
    IC u32 id() { return _id; }
private:
    virtual void add_ref()
    {
        ref_lock.Enter();
        ++_use_count;
        ref_lock.Leave();
    }
    virtual void free_ref()
    {
        ref_lock.Enter();
        R_ASSERT(_use_count > 0);
        --_use_count;
        if (_clear)
        {
            destroy_data();
            _clear = false;
        }
        ref_lock.Leave();
    }
    virtual void on_task_send(IGenericStream* outStream) const
    {
        R_ASSERT2(_id > 0, "data not ready call globaldata<type>::init()");
        //
        const xr_vector<e_net_globals>& v = gl_gl_reg().get_globals(gl_type);
        u32 size = v.size();
        for (u32 i = 0; i < size; ++i)
            globals().get(v[i]).on_task_send(outStream);
        //
        outStream->Write(&_id, sizeof(_id));
    }
    virtual bool on_task_receive(IAgent* agent, u32 sessionId, IGenericStream* inStream)
    {
        //
        const xr_vector<e_net_globals>& v = gl_gl_reg().get_globals(gl_type);
        u32 size = v.size();
        for (u32 i = 0; i < size; ++i)
            globals().get(v[i]).on_task_receive(agent, sessionId, inStream);
        //
        create_data_lock.Enter();
        u32 i;
        inStream->Read(&i, sizeof(i));
        R_ASSERT(i > 0);
        if (i == _id)
        {
            create_data_lock.Leave();
            return true;
        }
        if (_id == 0)
        {
            bool ret = create_data(i, agent, sessionId);
            create_data_lock.Leave();
            return ret;
        }
        create_data_lock.Leave();
        return false;
    }

    virtual LPCSTR files(string_path& buf)
    {
        //

        const xr_vector<e_net_globals>& v = gl_gl_reg().get_globals(gl_type);
        // xr_vector<e_net_globals>::const_iterator i = v.begin(), e = v.end();
        u32 size = v.size();
        buf[0] = 0;
        for (u32 i = 0; i < size; ++i)
        {
            string_path lbuf;
            strconcat(sizeof(string256), buf, buf, globals().get(v[i]).files(lbuf));
        }
        strconcat(sizeof(string256), buf, buf, ",", impl::file_name());
        return buf;
    }

    void create_data_file()
    {
        R_ASSERT(_id > 0);
        string_path path_name;
        global_data_file_path(impl::file_name(), path_name);
        impl::create_data_file(path_name);
    }

    bool create_data(u32 id, IAgent* agent, u32 sessionId)
    {
        if (_id == id)
            return true;
        string_path path_name;
        if (!global_data_file_path(impl::file_name(), agent, sessionId, path_name))
            return false;
        if (impl::create_data(path_name))
            _id = id;
        return _id == id;
    }
    void destroy_data()
    {
        R_ASSERT(_use_count == 0);
        impl::destroy_data();
        _id = 0;
    }
    virtual void data_init()
    {
        data_cleanup();
        create_data_lock.Enter();
        impl::data_init();
        ++_id;
        create_data_file();
        create_data_lock.Leave();
    }

    virtual void data_cleanup()
    {
        create_data_lock.Enter();

        if (_id > 0)
        {
            R_ASSERT(_use_count == 0);
            impl::data_cleanup();
            lc_net::cleanup().set_cleanup<gl_type>(_id);
        }
        create_data_lock.Leave();
    }
};

template <e_net_globals i>
struct it
{
    static const e_net_globals et = (e_net_globals)(i);
    static const e_net_globals next_et = (e_net_globals)(i + 1);
    typedef it<next_et> next;
    next ni;
    it(xr_vector<net_global_data*>& data) : ni(data) { data[et] = xr_new<tnet_global_data<et>>(); }
    static void cleanup(xr_vector<net_global_data*>& data)
    {
        tnet_global_data<et>* gd = static_cast<tnet_global_data<et>*>(data[et]);
        if (gd->id() > 0 && lc_net::cleanup().get_cleanup<et>() >= gd->id())
            gd->clear();
        next::cleanup(data);
    }
};
template <>
struct it<gl_last>
{
    it(xr_vector<net_global_data*>& data) {}
    static void cleanup(xr_vector<net_global_data*>& data)
    {
#ifdef CL_NET_LOG
        Msg("clean up end call");
#endif
    }
};

net_globals::net_globals()
{
    data.resize(gl_last, 0);
    it<gl_cl_data> i(data);
}

void net_globals::cleanup()
{
#ifdef CL_NET_LOG
    Msg("globals cleanup call");
#endif
    it<gl_cl_data>::cleanup(data);
}
}
