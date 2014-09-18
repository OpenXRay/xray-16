#ifndef	_XRLIGHT_IMPLICIED_H_
#define	_XRLIGHT_IMPLICIED_H_
class INetReader ;
class net_task_callback;
class ImplicitExecute
{
	// Data for this thread
	u32					y_start,y_end;
public:
	ImplicitExecute( u32 _y_start, u32 _y_end ): y_start(_y_start), y_end( _y_end )
	{
	
	}
	ImplicitExecute(): y_start( u32(-1) ),y_end( u32(-1) )
	{

	}
	void		Execute			( net_task_callback *net_callback );
	void		read			( INetReader	&r );
	void		write			( IWriter	&w ) const ;

	void		receive_result			( INetReader	&r );
	void		send_result				( IWriter	&w ) const ;
};


#endif