#include "ximacfg.h"

#define XRCORE_API XR_IMPORT
#include "xrCore/xrMemory.h"

void*	cxalloc(size_t size)
{
	return xr_malloc(size);
}

void	cxfree(void* ptr)
{
	return xr_free(ptr);
}

void*	cxrealloc(void* ptr, size_t size)
{
	return xr_realloc(ptr, size);
}

#undef XRCORE_API
