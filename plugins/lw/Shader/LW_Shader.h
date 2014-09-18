#ifndef _LW_SHADER_H_
#define _LW_SHADER_H_
#pragma once

#include "..\lw_shared\LW_ShaderDef.h"

typedef char sh_name[64];

typedef struct st_EShaderList{
	int			count;
	sh_name		Names[1024];
}EShaderList;

#endif