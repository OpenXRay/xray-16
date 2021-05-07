#include "ximacfg.h"

#define XRCORE_API XR_IMPORT
#include "xrCore/xrMemory.h"

#ifdef CXIMAGE_AS_SHARED_LIBRARY
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

#endif //#ifdef CXIMAGE_AS_SHARED_LIBRARY
