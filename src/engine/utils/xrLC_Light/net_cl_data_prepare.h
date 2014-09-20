#ifndef	_NET_CL_DATA_PREPARE_H_
#define	_NET_CL_DATA_PREPARE_H_
	XRLC_LIGHT_API  void				SetGlobalCompileDataInitialized( );
	XRLC_LIGHT_API  void				SetGlobalLightmapsDataInitialized( );
	XRLC_LIGHT_API  void				SartupNetTaskManager( );
	XRLC_LIGHT_API  void				RunNetCompileDataPrepare( );
	XRLC_LIGHT_API  void				WaitNetCompileDataPrepare( );
					void				SetRefModelLightDataInitialized( );
					void				WaitNetBaseCompileDataPrepare( );
#endif