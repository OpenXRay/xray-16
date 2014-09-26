#ifndef _LW_SHADERDEF_H_
#define _LW_SHADERDEF_H_
#pragma once

/* our instance data */
#pragma pack(push,1)
typedef struct st_XRShader{
	char	en_name[64];
	int		en_idx;
	char	lc_name[64];
	int		lc_idx;
	char	gm_name[64];
	int		gm_idx;
	char*	desc;
} XRShader;
#pragma pack(pop)

#define SH_PLUGIN_NAME "!XRayShader"

#endif
