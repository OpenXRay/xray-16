#ifndef	_MU_MODEL_LIGHT_H_
#define	_MU_MODEL_LIGHT_H_

extern XRLC_LIGHT_API	void	run_mu_light		( bool net );
extern XRLC_LIGHT_API	void	wait_mu_base		();
extern XRLC_LIGHT_API	void	wait_mu_secondary	();
extern XRLC_LIGHT_API	void	wait_mu_secondary_thread();
extern XRLC_LIGHT_API	void	WaitMuModelsLocalCalcLightening();
extern bool	mu_light_net;
#endif