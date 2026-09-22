#define RTGI_PROFILE_LIGHTING_SUN
#define BINDLESS_NO_IMPLICIT_GRAD
#include "bindless_common.h"
#include "rtgi_profile_lighting.h"

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    RTGIProfileLightingStage(dispatchID);
}
