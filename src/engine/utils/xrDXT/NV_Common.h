/*********************************************************************NVMH2****
Path:  c:\Dev\devrel\Nv_sdk_4\Include
File:  NV_Common.h

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:


******************************************************************************/



#ifndef	__NV_COMMON_H
#define	__NV_COMMON_H


#include <windows.h>
#include <assert.h>




#ifndef ASSERT_IF_FAILED
#define ASSERT_IF_FAILED( hres )	\
{									\
	if( FAILED(hres) )				\
	   assert( false );				\
}
#endif

#ifndef	ASSERT_AND_RET_IF_FAILED
#define ASSERT_AND_RET_IF_FAILED(hres)	\
{										\
	if( FAILED(hres) )					\
	{									\
		assert( false );				\
		return( hres );					\
	}									\
}
#endif


#ifndef FAIL_IF_NULL
#define FAIL_IF_NULL( x )  { if( x==NULL ) { assert(false); return(E_FAIL); } }
#endif


	// FDebug defined in NV_Error.h
#ifndef ASSERT_MSG
	#define ASSERT_MSG( var, msg )	\
	{									\
		if( !(var) )					\
		{								\
			FDebug( msg );				\
		}								\
		assert(var);					\
	}
#endif



#ifndef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(p)  { if(p) { delete [] (p);  p=NULL; } }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif
  
///////////////////////////////////////////////////////////////


#define ifnot(x)  if (!(x))
#define until(x) while(!(x))
#define ever          (;;)
#define wait        do {}
#define nothing     {}


///////////////////////////////////////////////////////////////
// @@ extract more of these from ulCommon.h

typedef unsigned short		USHORT;
typedef unsigned short		ushort;


#endif
