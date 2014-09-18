#pragma once

class XRLC_LIGHT_API CThread
{
	static void			startup(void* P);
public:
	volatile u32		thID;
	volatile float		thProgress;
	volatile BOOL		thCompleted;
	volatile BOOL		thMessages;
	volatile BOOL		thMonitor;
	volatile float		thPerformance;
	volatile BOOL		thDestroyOnComplete;

	CThread				(u32 _ID)	
	{
		thID				= _ID;
		thProgress			= 0;
		thCompleted			= FALSE;
		thMessages			= TRUE;
		thMonitor			= FALSE;
		thDestroyOnComplete	= TRUE;
	}
	virtual				~CThread(){}
	void				Start	()
	{
		thread_spawn	(startup,"worker-thread",1024*1024,this);
	}
	virtual		void	Execute	()	= 0;
};

class XRLC_LIGHT_API CThreadManager
{
	xr_vector<CThread*>	threads;
public:
	void				start	(CThread*	T);
	void				wait	(u32		sleep_time=1000);
};


IC void get_intervals( u32 max_threads, u32 num_items, u32 &threads, u32 &stride, u32 &rest )
{
	if(max_threads<=num_items)
	{
		threads	= max_threads;
		stride	= num_items/max_threads;
		rest	= num_items%max_threads;
		return;
	}
	threads		= num_items;
	stride		= 1;
	rest		= 0;
}