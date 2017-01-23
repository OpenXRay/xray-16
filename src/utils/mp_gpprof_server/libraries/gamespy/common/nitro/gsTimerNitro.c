// NITRO
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../gsCommon.h"

// note that this doesn't return the standard time() value
// because the DS doesn't know what timezone it's in
time_t time(time_t *timer)
{
	time_t t;

	assert(OS_IsTickAvailable() == TRUE);
	t = (time_t)OS_TicksToSeconds(OS_GetTick());

	if(timer)
		*timer = t;

	return t;
}