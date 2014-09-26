// This code is in the public domain -- castano@gmail.com

#ifndef NV_CORE_LIBRARY_H
#define NV_CORE_LIBRARY_H

#include <nvcore/nvcore.h>

#if NV_OS_WIN32
#define LIBRARY_NAME(name)	#name ".dll"
#elif NV_OS_DARWIN
#define NV_LIBRARY_NAME(name)	"lib" #name ".dylib"
#else
#define NV_LIBRARY_NAME(name)	"lib" #name ".so"
#endif

NVCORE_API void * nvLoadLibrary(const char * name);
NVCORE_API void nvUnloadLibrary(void * lib);
NVCORE_API void * nvBindSymbol(void * lib, const char * symbol);

class NVCORE_CLASS Library
{
public:
	Library(const char * name)
	{
		handle = nvLoadLibrary(name);
	}
	~Library()
	{
		if (isValid())
		{
			nvUnloadLibrary(handle);
		}
	}
	
	bool isValid() const
	{
		return handle != NULL;
	}
	
	void * bindSymbol(const char * symbol)
	{
		return nvBindSymbol(handle, symbol);
	}
	
private:
	void * handle;
};


#endif // NV_CORE_LIBRARY_H
