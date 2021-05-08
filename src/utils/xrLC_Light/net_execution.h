#ifndef _NET_EXECUTION_H_
#define _NET_EXECUTION_H_
#include "hxgrid/Interface/IAgent.h"
#include "hxgrid/Interface/hxgridinterface.h"

#include "net_global_data.h"
#include "net_task_callback.h"
namespace lc_net
{
class net_execution
{
    u32 _id;

public:
    u32 id() const { return _id; }
    virtual u32 type() = 0;

    virtual void send_task(IGridUser& user, IGenericStream* outStream, u32 id) = 0;
    virtual void receive_result(IGenericStream* outStream) = 0;
    virtual bool receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream) = 0;
    virtual void send_result(IGenericStream* outStream) = 0;
    virtual bool execute(IAgent* agent, u32 sessionId) = 0;
    virtual LPCSTR data_files(string_path& buf) = 0;

    net_execution(u32 id) : _id(id) {}
    virtual ~net_execution(){};

private:
};

enum execution_types
{
    et_lightmaps,
    et_implicit_light,
    et_mu_ref_light,
    et_mu_base_light,
    et_vertex_light,
    et_detail_light,
    et_last
};

template <execution_types etype>
class tnet_execution_base : public net_execution
{
public:
    static const execution_types class_type = etype;
    class net_execution_impl;

public:
    tnet_execution_base(u32 id) : net_execution(id) {}
    virtual net_execution_impl& implementation() = 0;

private:
    virtual u32 type() { return (u32)class_type; }
};

template <execution_types ie, e_net_globals ig>
struct add_global;
};
#endif
