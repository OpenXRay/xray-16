#ifndef        COMMON_H
#define        COMMON_H

// #define USE_SUPER_SPECULAR

#include "shared\common.h"
#include "anomaly_shaders.h"
#include "common_defines.h"
#include "common_policies.h"
#include "common_iostructs.h"
#include "common_samplers.h"
#include "common_functions.h"
#include "gbuffer_stage.h"

#define USE_SUNMASK                		//- shader defined

#ifdef        USE_R2_STATIC_SUN
#  define xmaterial float(1.0h/4.h)
#else
#  define xmaterial float(L_material.w)
#endif

#define FXVS technique _render{pass _code{VertexShader=compile vs_3_0 main();}}

#endif