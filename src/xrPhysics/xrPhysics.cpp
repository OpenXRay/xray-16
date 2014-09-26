// xrPhysics.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "xrPhysics.h"

#include "../3rd party/ode/include/ode/memory.h"

#pragma comment(lib,"ode.lib")
#pragma comment(lib,"xrCDB.lib")
#pragma comment( lib, "xrSound.lib"	)
#pragma comment( lib, "xrAPI.lib"	)
//#pragma comment(lib,"xrEngine.lib")




#ifdef _MANAGED
#pragma managed(push, off)
#endif


#ifdef DEBUG_MEMORY_MANAGER
	static	void *	ode_alloc	(size_t size)								{ return Memory.mem_alloc(size,"ODE");			}
	static	void *	ode_realloc	(void *ptr, size_t oldsize, size_t newsize)	{ return Memory.mem_realloc(ptr,newsize,"ODE");	}
	static	void	ode_free	(void *ptr, size_t size)					{ return xr_free(ptr);							}
#else // DEBUG_MEMORY_MANAGER
	static	void *	ode_alloc	(size_t size)								{ return xr_malloc(size);			}
	static	void *	ode_realloc	(void *ptr, size_t oldsize, size_t newsize)	{ return xr_realloc(ptr,newsize);	}
	static	void	ode_free	(void *ptr, size_t size)					{ return xr_free(ptr);				}
#endif // DEBUG_MEMORY_MANAGER




BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
   	lpReserved;
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:

			dSetAllocHandler			(ode_alloc		);
			dSetReallocHandler			(ode_realloc	);
			dSetFreeHandler				(ode_free		);

			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;

}

#ifdef _MANAGED
#pragma managed(pop)
#endif

