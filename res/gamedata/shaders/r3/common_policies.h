#ifndef	common_policies_h_included
#define	common_policies_h_included

//	Define default sample index for MSAA
#ifndef	ISAMPLE
#define ISAMPLE 0
#endif	//	ISAMPLE

//	redefine sample index
#ifdef 	MSAA_OPTIMIZATION
#undef	ISAMPLE
#define ISAMPLE	iSample
#endif	//	MSAA_OPTIMIZATION

/////////////////////////////////////////////////////////////////////////////
// GLD_P - gbuffer_load_data
	#define	GLD_P( _tc, _pos2d, _iSample ) _tc, _pos2d, _iSample


/////////////////////////////////////////////////////////////////////////////
// CS_P
#ifdef USE_MSAA
#		define	CS_P( _P, _N, _tc0, _tcJ, _pos2d, _iSample ) _P, _N, _tc0, _tcJ, _pos2d, _iSample
#else
#		define	CS_P( _P, _N, _tc0, _tcJ, _pos2d, _iSample ) _P, _N, _tc0, _tcJ, _pos2d
#endif

#endif	//	common_policies_h_included
