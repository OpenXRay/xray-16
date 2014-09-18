#include "stdafx.h"
#include "net_cl_data_prepare.h"

#include "lc_net_global_data.h"
#include "gl_base_cl_data.h"
#include "lm_net_global_data.h"
#include "ref_model_net_global_data.h"
#include "lcnet_task_manager.h"
#include "xrlc_globaldata.h"
#include "mu_light_net.h"
#include "xrThread.h"
#include "../../xrcore/xrSyncronize.h"

bool					global_compile_data_initialized = false;
bool					base_global_compile_data_initialized = false;
CThreadManager			cl_data_prepare;
xrCriticalSection		wait_lock;
void		SetBaseGlobalCompileDataInitialized( );
class NetCompileDetaPrepare	: public CThread
{

public:
	NetCompileDetaPrepare	( ) : CThread( 0 )	{	thMessages	= FALSE;	}
private:
	virtual void	Execute	()
	{
		SetBaseGlobalCompileDataInitialized( );
		SetGlobalCompileDataInitialized( );
	}

};


void		RunNetCompileDataPrepare( )
{
	cl_data_prepare.start( xr_new<NetCompileDetaPrepare>() );
	SartupNetTaskManager( );//.
}

void		WaitNetCompileDataPrepare( )
{
	for(;;)
	{
		Sleep(1000);
		bool inited = false;
		wait_lock.Enter();
		//cl_data_prepare.wait();
		inited = global_compile_data_initialized;
		wait_lock.Leave();
		if(inited)
			break;
	}
}
void		WaitNetBaseCompileDataPrepare( )//to do refactoring
{
	for(;;)
	{
		Sleep(1000);
		bool inited = false;
		wait_lock.Enter();
		//cl_data_prepare.wait();
		inited = base_global_compile_data_initialized;
		wait_lock.Leave();
		if(inited)
			break;
	}
}

void		SetBaseGlobalCompileDataInitialized( )
{
	
	lc_net::globals().get<lc_net::gl_base_cl_data>().init();
	wait_lock.Enter();
	base_global_compile_data_initialized = true;
	wait_lock.Leave();
	
}

void		SetGlobalCompileDataInitialized( )
{
	
	lc_net::globals().get<lc_net::gl_cl_data>().init();
	clLog( "mem usage before collision model destroy: %u", Memory.mem_usage() );
	inlc_global_data()->destroy_rcmodel	();
	Memory.mem_compact();
	clLog( "mem usage after collision model destroy: %u", Memory.mem_usage() );
//	inlc_global_data()->clear_build_textures_surface();
	wait_lock.Enter();
		//cl_data_prepare.wait();
	global_compile_data_initialized = true;
	wait_lock.Leave();
	
}

void		SartupNetTaskManager( )
{
	lc_net::get_task_manager().startup();
}

extern u32		vertises_has_lighting;
u32 CalcAllTranslucency();

void		SetGlobalLightmapsDataInitialized( )
{
	WaitNetCompileDataPrepare( );
//
	vertises_has_lighting = CalcAllTranslucency();
//
	lc_net::globals().get<lc_net::gl_lm_data>().init();
	
}

void		SetRefModelLightDataInitialized( )
{
	WaitNetCompileDataPrepare( );
	lc_net::WaitBaseModelsNet( );
	lc_net::globals().get<lc_net::gl_ref_model_data>().init();
}