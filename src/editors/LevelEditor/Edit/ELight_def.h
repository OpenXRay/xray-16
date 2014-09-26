//----------------------------------------------------
// file: ELightDef.h
//----------------------------------------------------

#ifndef ELightDefH
#define ELightDefH

#define LCONTROL_HEMI			"$hemi"				// hemisphere
#define LCONTROL_SUN			"$sun"				// sun
#define LCONTROL_STATIC 		"$static"			// all other static lights

namespace ELight{
	enum EFlags{
		flAffectStatic	= (1<<0),
		flAffectDynamic	= (1<<1),
		flProcedural	= (1<<2),
        flBreaking 		= (1<<3),
        flPointFuzzy	= (1<<4),
        flCastShadow	= (1<<5),
    };

    enum EType{
    	ltPoint		= D3DLIGHT_POINT,
    	ltSpot  	= D3DLIGHT_SPOT,
        ltDirect	= D3DLIGHT_DIRECTIONAL,
        ltMaxCount,
    	lt_max_type = u32(-1),
    };
};

#endif

