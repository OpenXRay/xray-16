#ifndef _CLSID_H
#define _CLSID_H

#pragma once

//***** CLASS ID type
typedef u64		CLASS_ID;
#define MK_CLSID(a,b,c,d,e,f,g,h) \
    	CLASS_ID((CLASS_ID(a)<<CLASS_ID(56))|(CLASS_ID(b)<<CLASS_ID(48))|(CLASS_ID(c)<<CLASS_ID(40))|(CLASS_ID(d)<<CLASS_ID(32))|(CLASS_ID(e)<<CLASS_ID(24))|(CLASS_ID(f)<<CLASS_ID(16))|(CLASS_ID(g)<<CLASS_ID(8))|(CLASS_ID(h)))

#define MK_CLSID_INV(a,b,c,d,e,f,g,h) MK_CLSID(h,g,f,e,d,c,b,a)

extern XRCORE_API void		__stdcall CLSID2TEXT(CLASS_ID id, LPSTR text);
extern XRCORE_API CLASS_ID	__stdcall TEXT2CLSID(LPCSTR text);

#endif
