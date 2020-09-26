#ifndef _NET_GLOBAL_DATA_H_
#define _NET_GLOBAL_DATA_H_
#include "hxgrid/Interface/hxgridinterface.h"
namespace lc_net
{
enum e_net_globals
{
    gl_cl_data,
    gl_implicit_cl_data,
    gl_lm_data,
    gl_ref_model_data,
    gl_detail_cl_data,
    gl_base_cl_data,
    gl_last
};

class net_global_data
{
public:
    virtual void on_task_send(IGenericStream* outStream) const = 0;
    virtual bool on_task_receive(IAgent* agent, u32 sessionId, IGenericStream* inStream) = 0;
    virtual LPCSTR files(string_path& buf) = 0;
    virtual void add_ref() = 0;
    virtual void free_ref() = 0;
};
template <e_net_globals gl_type>
class net_global_data_impl;

template <e_net_globals gl_type>
class tnet_global_data_base : public net_global_data, public net_global_data_impl<gl_type>
{
};

class net_globals
{
    xr_vector<net_global_data*> data;

public:
    net_globals();

    net_global_data& get(e_net_globals id) { return *data[id]; }
    template <e_net_globals gl_type>
    net_global_data_impl<gl_type>& get()
    {
        return *static_cast<net_global_data_impl<gl_type>*>(
            static_cast<tnet_global_data_base<gl_type>*>(data[gl_type]));
    }

private:
    friend class global_data_cleanup;
    void cleanup();
};

net_globals& globals();
template <e_net_globals ie, e_net_globals ig>
struct global_add_global;
}
#endif
