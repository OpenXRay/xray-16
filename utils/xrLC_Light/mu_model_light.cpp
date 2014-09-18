#include "stdafx.h"

#include "mu_model_light.h"
#include "mu_model_light_threads.h"
#include "mu_light_net.h"
extern bool	mu_light_net = false;
void	run_mu_light		( bool net )
{
	mu_light_net   = net;

	run_mu_base		( net ); 
 

}
void	wait_mu_base		()
{
	wait_mu_base_thread		();
}
void	wait_mu_secondary	()
{
	if(!mu_light_net)
		wait_mu_secondary_thread();
	else
		lc_net::WaitRefModelsNet( );

}



