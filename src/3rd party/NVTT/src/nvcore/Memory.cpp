
#include "Memory.h"
#include "Debug.h"

//#if HAVE_MALLOC_H
//#include <malloc.h>
//#endif

#include <stdlib.h>


using namespace nv;

void * nv::mem::malloc(size_t size)
{
	return ::malloc(size);
}

void * nv::mem::malloc(size_t size, const char * file, int line)
{
	NV_UNUSED(file);
	NV_UNUSED(line);
	return ::malloc(size);
}

void nv::mem::free(const void * ptr)
{
	::free(const_cast<void *>(ptr));
}

void * nv::mem::realloc(void * ptr, size_t size)
{
	nvDebugCheck(ptr != NULL || size != 0);	// undefined realloc behavior.
	return ::realloc(ptr, size);
}

