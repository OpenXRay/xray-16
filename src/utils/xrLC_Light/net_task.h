#pragma once

#include "light_execute.h"
#include "net_task_callback.h"
// class IWriter;
// class INetReader;
/*
class net_task
{
    INetReader		*in_stream  ;
    IWriter			*out_stream ;
public:
    net_task(  INetReader* inStream,  IWriter* outStream ): in_stream( inStream ), out_stream( outStream )
    {
        VERIFY(in_stream);
        VERIFY(out_stream);
    };
    virtual void run()	= 0;
};

*/

class net_task : public net_task_callback
{
    CDeflector* _D;
    u32 _id;
    light_execute _execute;

public:
    void run();

    bool receive(IGenericStream* inStream);
    bool send(IGenericStream* outStream);

    net_task(IAgent* agent, u32 session);
    ~net_task();
};
