/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: compilation parameters and miscelaneous definitions           *
 *  Comments: system & compiler dependent file                              *
 ****************************************************************************/
#if !defined(_PPMDTYPE_H_)
#define _PPMDTYPE_H_

#include <stdio.h>

#if defined(XR_PLATFORM_WINDOWS)
#define _WIN32_ENVIRONMENT_
//#define _DOS32_ENVIRONMENT_
//#define _POSIX_ENVIRONMENT_
//#define _UNKNOWN_ENVIRONMENT_
#if defined(_WIN32_ENVIRONMENT_) + defined(_DOS32_ENVIRONMENT_) + defined(_POSIX_ENVIRONMENT_) + \
defined(_UNKNOWN_ENVIRONMENT_) != \
    \
\
1
#error Only one environment must be defined
#endif /* defined(_WIN32_ENVIRONMENT_)+defined(_DOS32_ENVIRONMENT_)+defined(_POSIX_ENVIRONMENT_)+defined(_UNKNOWN_ENVIRONMENT_) \
          \                                                                                                                     \
          \ \                                                                                                                              \
          != 1 */
#endif
#if defined(_WIN32_ENVIRONMENT_)
#include <windows.h>
#else /* _DOS32_ENVIRONMENT_ || _POSIX_ENVIRONMENT_ || _UNKNOWN_ENVIRONMENT_ */
#include "stdafx.h"
//typedef int BOOL;
//#define FALSE 0
//#define TRUE 1
//typedef unsigned char BYTE;
//typedef unsigned short WORD;
//typedef unsigned long DWORD;
//typedef unsigned int UINT;
#endif /* defined(_WIN32_ENVIRONMENT_)  */

const u32 PPMdSignature = 0x84ACAF8F, Variant = 'I';
const int MAX_O = 16; /* maximum allowed model order  */

#define _USE_PREFETCHING /* for puzzling mainly          */

#if !defined(_UNKNOWN_ENVIRONMENT_) && !defined(__GNUC__)
#define _FASTCALL __fastcall
#define _STDCALL __stdcall
#else
#define _FASTCALL
#define _STDCALL
#endif /* !defined(_UNKNOWN_ENVIRONMENT_) && !defined(__GNUC__) */

#if defined(__GNUC__)
#define _PACK_ATTR __attribute__((packed))
#else /* "#pragma pack" is used for other compilers */
#define _PACK_ATTR
#endif /* defined(__GNUC__) */

/* PPMd module works with file streams via ...GETC/...PUTC macros only      */
#ifndef VERIFY
#define VERIFY(a)
#define VERIFY2(a, b)
#define VERIFY_is_defined_in_ppmdtype_h
#endif // VERIFY
typedef unsigned char u8;
typedef unsigned int u32;
#include "compression_ppmd_stream.h"

#ifdef VERIFY_is_defined_in_ppmdtype_h
#undef VERIFY
#undef VERIFY2
#undef VERIFY_is_defined_in_ppmdtype_h
#endif // VERIFY_is_defined_in_ppmdtype_h

#include "compression_ppmd_stream.h"
typedef compression::ppmd::stream _PPMD_FILE;
#define _PPMD_E_GETC(fp) fp->get_char()
#define _PPMD_E_PUTC(c, fp) fp->put_char((c))
#define _PPMD_D_GETC(fp) fp->get_char()
#define _PPMD_D_PUTC(c, fp) fp->put_char((c))
/******************  Example of C++ buffered stream  ************************
class PRIME_STREAM {
public:
enum { BUF_SIZE=64*1024 };
    PRIME_STREAM(): Error(0), StrPos(0), Count(0), p(Buf) {}
    int  get(     ) { return (--Count >= 0)?(*p++    ):( fill( )); }
    int  put(int c) { return (--Count >= 0)?(*p++ = c):(flush(c)); }
    int  getErr() const { return Error; }
    int    tell() const { return StrPos+(p-Buf); }
    BOOL  atEOS() const { return (Count < 0); }
protected:
    int Error, StrPos, Count;
    BYTE* p, Buf[BUF_SIZE];
    virtual int  fill(     ) = 0;           // it must fill Buf[]
    virtual int flush(int c) = 0;           // it must remove (p-Buf) bytes
};
typedef PRIME_STREAM _PPMD_FILE;
#define _PPMD_E_GETC(pps)   (pps)->get()
#define _PPMD_E_PUTC(c,pps) (pps)->put(c)
#define _PPMD_D_GETC(pps)   (pps)->get()
#define _PPMD_D_PUTC(c,pps) (pps)->put(c)
**************************  End of example  *********************************/

#endif /* !defined(_PPMDTYPE_H_) */
