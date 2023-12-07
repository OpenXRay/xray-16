#include <stdio.h>
#include <cstring>
#include <stdlib.h> // for malloc
#include <unistd.h> // for rmdir
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h> // for PAGESIZE...
#include <math.h>
#include <sched.h>

#include <algorithm> // for min max
#include <stddef.h>

#include <string>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h> // for mmap / munmap
#include <dirent.h>
#include <utime.h>
#include <ctime>
#include <fenv.h>

#pragma STDC FENV_ACCESS ON

#define _LINUX // for GameSpy

#define _MAX_PATH PATH_MAX + 1
#define MAX_PATH PATH_MAX + 1

#define WINAPI

#define _copysign copysign

#define _cdecl //__attribute__((cdecl))
#define _stdcall //__attribute__((stdcall))
#define _fastcall //__attribute__((fastcall))

#define __cdecl
#define __stdcall

//#define __declspec
#define __forceinline FORCE_INLINE
#define __pragma(...) _Pragma(#__VA_ARGS__)
#define __declspec(x)
#define CALLBACK
#define TEXT(x) strdup(x)



#define VOID void
#define HKL void*
#define ActivateKeyboardLayout(x, y) {}
#define ScreenToClient(hwnd, p) {}

#define __except(X) catch(X)

#define GetCurrentProcessId getpid
#define GetCurrentThreadId pthread_self

inline void Sleep(int ms)
{
    usleep(ms * 1000);
}

inline void _splitpath(const char* path, // Path Input
        char* drive, // Drive     : Output
        char* dir, // Directory : Output
        char* fname, // Filename  : Output
        char* ext // Extension : Output
        )
{
    if(!path)
        return;

    const char *p, *end;

    if(drive)
        strcpy(drive, "");

    end = NULL;
    for(p = path; *p; p++)
        if(*p == '/' || *p == '\\')
            end = p + 1;

    if(end)
    {
        if(dir)
        {
            memcpy(dir, path, end - path);
            dir[end - path] = 0;
        }
        path = end;
    }
    else if(dir)
        dir[0] = 0;

    end = strchr(path, '.');

    if(!end)
        end = p;

    if(fname)
    {
        memcpy(fname, path, end - path);
        fname[end - path] = 0;
    }

    if(ext)
        strcpy(ext, end);
}

#include <iostream>
inline void OutputDebugString(const char *str) // for linux debugger
{
    std::cerr << str;
}

inline unsigned long GetLastError()
{
    return 0;
}

inline int GetExceptionCode()
{
    return 0;
}

#include <inttypes.h>
typedef int32_t BOOL;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
#ifndef _LIBRAW_TYPES_H
typedef int64_t INT64;
typedef uint64_t UINT64;
#endif

typedef char* LPSTR;
typedef char* PSTR;
typedef char* LPTSTR;
typedef const char* LPCSTR;
typedef const char* LPCTSTR;
typedef unsigned char* LPBYTE;
typedef unsigned int UINT;
typedef int INT;
typedef unsigned long ULONG;
typedef unsigned long* ULONG_PTR;
typedef long long int LARGE_INTEGER;
typedef unsigned long long int ULARGE_INTEGER;

typedef unsigned short* LPWORD;
typedef unsigned long* LPDWORD;
typedef const void* LPCVOID;
typedef long long int* PLARGE_INTEGER;

typedef wchar_t WCHAR;

#define WAVE_FORMAT_PCM        0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003

typedef struct {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX, *LPWAVEFORMATEX;

typedef struct tagSTICKYKEYS
{
    DWORD   cbSize;
    DWORD   dwFlags;
} STICKYKEYS, *LPSTICKYKEYS;

typedef struct tagFILTERKEYS
{
    UINT   cbSize;
    DWORD  dwFlags;
    DWORD  iWaitMSec;
    DWORD  iDelayMSec;
    DWORD  iRepeatMSec;
    DWORD  iBounceMSec;
} FILTERKEYS, *LPFILTERKEYS;

typedef struct tagTOGGLEKEYS
{
    DWORD   cbSize;
    DWORD   dwFlags;
} TOGGLEKEYS, *LPTOGGLEKEYS;

typedef struct _EXCEPTION_POINTERS {
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

#if defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_ARM64)
typedef int64_t INT_PTR;
typedef uint64_t UINT_PTR;
typedef int64_t LONG_PTR;
#else
typedef int INT_PTR;
typedef unsigned int UINT_PTR;
typedef long LONG_PTR;
#endif // XR_ARCHITECTURE_X64

typedef void* HANDLE;
typedef void* HMODULE;
typedef void* PVOID;
typedef void* LPVOID;
#if defined(XR_ARCHITECTURE_PPC64)
typedef LONG_PTR WPARAM;
#else
typedef UINT_PTR WPARAM;
#endif
typedef LONG_PTR LPARAM;
typedef long HRESULT;
typedef long LRESULT;
typedef long _W64;
typedef void* HWND;
typedef void* HDC;
typedef float FLOAT;
typedef unsigned char UINT8;

typedef struct _RECT {
    long left;
    long top;
    long right;
    long bottom;
} RECT, *PRECT;

typedef struct tagPOINT {
    long x;
    long y;
} POINT, *PPOINT, *LPPOINT;

#define DWORD_PTR UINT_PTR
#define WM_USER 0x0400

#define TRUE true
#define FALSE false
#define NONE 0
#define CONST const

typedef dirent DirEntryType;

#define _O_WRONLY O_WRONLY
#define _O_RDONLY O_RDONLY
#define _O_TRUNC O_TRUNC
#define _O_CREAT O_CREAT
#define _S_IWRITE S_IWRITE
#define _S_IREAD S_IREAD
#define _O_BINARY 0
#define O_BINARY 0
#define O_SEQUENTIAL 0
#define SH_DENYWR 0

#if __has_include(<SDL_stdinc.h>)
#include <SDL_stdinc.h>
#define itoa SDL_itoa
#define _itoa_s SDL_itoa
#else
#define itoa(...) do { static_assert(false, "SDL_stdinc.h is missing"); } while (false)
#define _itoa_s(...) do { static_assert(false, "SDL_stdinc.h is missing"); } while (false)
#endif

#define _stricmp stricmp
#define strcmpi stricmp
#define lstrcpy strcpy
#define stricmp strcasecmp
#define strupr SDL_strupr
// error code numbers from original MS strcpy_s return value
inline int strcpy_s(char *dest, size_t num, const char *source)
{
    if(!dest)
        return EINVAL;

    if(0 == num)
    {
        dest[0] = '\0';
        return ERANGE;
    }

    if(!source)
    {
        dest[0] = '\0';
        return EINVAL;
    }

    size_t i;
    for(i = 0; i < num; i++)
    {
        if((dest[i] = source[i]) == '\0')
            return 0;
    }
    dest[0] = '\0';
    return ERANGE;
}

template <std::size_t num>
inline int strcpy_s(char (&dest)[num], const char *source) { return strcpy_s(dest, num, source); }

inline int strncpy_s(char * dest, size_t dst_size, const char * source, size_t num)
{
    if (!dest || (0 == dst_size))
        return EINVAL;

    if(0 == num)
    {
        dest[0] = '\0';
        return 0;
    }

    if (!source)
    {
        dest[0] = '\0';
        return EINVAL;
    }

    size_t i, end;
    if(num < dst_size)
        end = num;
    else
        end = dst_size - 1;

    for(i = 0; i < end && source[i]; i++)
        dest[i] = source[i];

    if(!source[i] || end == num)
    {
        dest[i] = '\0';
        return 0;
    }

    dest[0] = '\0';

    return EINVAL;
}

template <std::size_t dst_sz>
inline int strncpy_s(char (&dest)[dst_sz], const char * source, size_t num) { return strncpy_s(dest, dst_sz, source, num); }

inline int strcat_s(char * dest, size_t num, const char * source)
{
    if(!dest)
        return EINVAL;

    if(!source)
    {
        dest[0] = '\0';
        return EINVAL;
    }

    size_t i, j;
    for(i = 0; i < num; i++)
    {
        if(dest[i] == '\0')
        {
            for(j = 0; (j + i) < num; j++)
            {
                if((dest[j + i] = source[j]) == '\0')
                    return 0;
            }
        }
    }

    dest[0] = '\0';
    return ERANGE;
}

inline int strncat_s(char * dest, size_t num, const char * source, size_t count)
{
    if (!dest || !source)
        return EINVAL;

    size_t i, j;
    for(i = 0; i < num; i++)
    {
        if(dest[i] == '\0')
        {
            for(j = 0; (j + i) < num; j++)
            {
                if(j == count || (dest[j + i] = source[j]) == '\0')
                {
                    dest[j + i] = '\0';
                    return 0;
                }
            }
        }
    }

    return ERANGE;
}

#define _vsnprintf vsnprintf
inline int vsnprintf_s(char* buffer, size_t size, size_t, const char* format, va_list list)
{
    //TODO add bound check
    return vsnprintf(buffer, size, format, list);
}
#define vsprintf_s(dest, size, format, args) vsprintf(dest, format, args)
#define _snprintf snprintf
#define sprintf_s(buffer, buffer_size, stringbuffer, ...) sprintf(buffer, stringbuffer, ##__VA_ARGS__)
//#define GetProcAddress(handle, name) dlsym(handle, name)
#define _chdir chdir
#define _strnicmp strnicmp
#define strnicmp strncasecmp
#define _getcwd getcwd
#define _snwprintf swprintf
#define swprintf_s swprintf
#define wcsicmp _wcsicmp
#define _wcsicmp wcscmp
#define _tempnam tempnam
#define _unlink unlink
#define _access access
#define _open open
#define _close close
#define _sopen open
#define _utime utime
#define _utimbuf utimbuf
#define _sopen_s(handle, filename, ...) open(filename, O_RDONLY)
inline int _filelength(int fd)
{
    struct stat file_info;
    ::fstat(fd, &file_info);
    return file_info.st_size;
}
#define _fdopen fdopen
#define _rmdir rmdir
#define _write write
#define _strupr strupr
#define _read read
#define _set_new_handler std::set_new_handler
#define _finite isfinite
inline int _mkdir(const char *dir) { return mkdir(dir, S_IRWXU); }

#define _wtoi(arg) wcstol(arg, NULL, 10)
#define _wtoi64(arg) wcstoll(arg, NULL, 10)
#undef min
#undef max
#define __max(a, b) std::max(a, b)
#define __min(a, b) std::min(a, b)

#define _locale_t locale_t
#define _isalpha_l isalpha_l
#define _create_locale(category, arg) newlocale(category, arg, (locale_t) 0)

#define ZeroMemory(p, sz) memset((p), 0, (sz))
#define CopyMemory(d, s, n) memcpy(d, s, n)

#define RGB(r,g,b) ( ((DWORD)(BYTE)r)|((DWORD)((BYTE)g)<<8)|((DWORD)((BYTE)b)<<16) )
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define S_OK 0x00000000
#define S_FALSE 0x10000000
#define E_FAIL 0x80004005

#define _MAX_DRIVE	3
#define _MAX_DIR	256
#define _MAX_FNAME	256
#define _MAX_EXT	256

#define SEM_FAILCRITICALERRORS 1
#define SetErrorMode(x) {}

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

typedef enum _D3DFORMAT {
    D3DFMT_UNKNOWN              =   0,

    D3DFMT_R8G8B8               =  20,
    D3DFMT_A8R8G8B8             =  21,
    D3DFMT_X8R8G8B8             =  22,
    D3DFMT_R5G6B5               =  23,
    D3DFMT_X1R5G5B5             =  24,
    D3DFMT_A1R5G5B5             =  25,
    D3DFMT_A4R4G4B4             =  26,
    D3DFMT_R3G3B2               =  27,
    D3DFMT_A8                   =  28,
    D3DFMT_A8R3G3B2             =  29,
    D3DFMT_X4R4G4B4             =  30,
    D3DFMT_A2B10G10R10          =  31,
    D3DFMT_A8B8G8R8             =  32,
    D3DFMT_X8B8G8R8             =  33,
    D3DFMT_G16R16               =  34,
    D3DFMT_A2R10G10B10          =  35,
    D3DFMT_A16B16G16R16         =  36,


    D3DFMT_A8P8                 =  40,
    D3DFMT_P8                   =  41,

    D3DFMT_L8                   =  50,
    D3DFMT_A8L8                 =  51,
    D3DFMT_A4L4                 =  52,

    D3DFMT_V8U8                 =  60,
    D3DFMT_L6V5U5               =  61,
    D3DFMT_X8L8V8U8             =  62,
    D3DFMT_Q8W8V8U8             =  63,
    D3DFMT_V16U16               =  64,
    D3DFMT_A2W10V10U10          =  67,

    D3DFMT_UYVY                 =  MAKEFOURCC('U', 'Y', 'V', 'Y'),
    D3DFMT_YUY2                 =  MAKEFOURCC('Y', 'U', 'Y', '2'),
    D3DFMT_DXT1                 =  MAKEFOURCC('D', 'X', 'T', '1'),
    D3DFMT_DXT2                 =  MAKEFOURCC('D', 'X', 'T', '2'),
    D3DFMT_DXT3                 =  MAKEFOURCC('D', 'X', 'T', '3'),
    D3DFMT_DXT4                 =  MAKEFOURCC('D', 'X', 'T', '4'),
    D3DFMT_DXT5                 =  MAKEFOURCC('D', 'X', 'T', '5'),
    D3DFMT_MULTI2_ARGB8         =  MAKEFOURCC('M', 'E', 'T', '1'),
    D3DFMT_G8R8_G8B8            =  MAKEFOURCC('G', 'R', 'G', 'B'),
    D3DFMT_R8G8_B8G8            =  MAKEFOURCC('R', 'G', 'B', 'G'),

    D3DFMT_D16_LOCKABLE         =  70,
    D3DFMT_D32                  =  71,
    D3DFMT_D15S1                =  73,
    D3DFMT_D24S8                =  75,
    D3DFMT_D24X8                =  77,
    D3DFMT_D24X4S4              =  79,
    D3DFMT_D16                  =  80,
    D3DFMT_L16                  =  81,
    D3DFMT_D32F_LOCKABLE        =  82,
    D3DFMT_D24FS8               =  83,

    D3DFMT_VERTEXDATA           = 100,
    D3DFMT_INDEX16              = 101,
    D3DFMT_INDEX32              = 102,
    D3DFMT_Q16W16V16U16         = 110,
    /* Floating point formats */
    D3DFMT_R16F                 = 111,
    D3DFMT_G16R16F              = 112,
    D3DFMT_A16B16G16R16F        = 113,

    /* IEEE formats */
    D3DFMT_R32F                 = 114,
    D3DFMT_G32R32F              = 115,
    D3DFMT_A32B32G32R32F        = 116,

    D3DFMT_CxV8U8               = 117,


    D3DFMT_FORCE_DWORD          = 0xFFFFFFFF
} D3DFORMAT;

typedef enum _D3DCULL {
    D3DCULL_NONE                = 1,
    D3DCULL_CW                  = 2,
    D3DCULL_CCW                 = 3,

    D3DCULL_FORCE_DWORD         = 0x7fffffff
} D3DCULL;

typedef enum _D3DCMPFUNC {
    D3DCMP_NEVER                = 1,
    D3DCMP_LESS                 = 2,
    D3DCMP_EQUAL                = 3,
    D3DCMP_LESSEQUAL            = 4,
    D3DCMP_GREATER              = 5,
    D3DCMP_NOTEQUAL             = 6,
    D3DCMP_GREATEREQUAL         = 7,
    D3DCMP_ALWAYS               = 8,

    D3DCMP_FORCE_DWORD          = 0x7fffffff
} D3DCMPFUNC;

typedef enum _D3DSTENCILOP {
    D3DSTENCILOP_KEEP           = 1,
    D3DSTENCILOP_ZERO           = 2,
    D3DSTENCILOP_REPLACE        = 3,
    D3DSTENCILOP_INCRSAT        = 4,
    D3DSTENCILOP_DECRSAT        = 5,
    D3DSTENCILOP_INVERT         = 6,
    D3DSTENCILOP_INCR           = 7,
    D3DSTENCILOP_DECR           = 8,

    D3DSTENCILOP_FORCE_DWORD    = 0x7fffffff
} D3DSTENCILOP;

typedef enum _D3DBLEND {
    D3DBLEND_ZERO               =  1,
    D3DBLEND_ONE                =  2,
    D3DBLEND_SRCCOLOR           =  3,
    D3DBLEND_INVSRCCOLOR        =  4,
    D3DBLEND_SRCALPHA           =  5,
    D3DBLEND_INVSRCALPHA        =  6,
    D3DBLEND_DESTALPHA          =  7,
    D3DBLEND_INVDESTALPHA       =  8,
    D3DBLEND_DESTCOLOR          =  9,
    D3DBLEND_INVDESTCOLOR       = 10,
    D3DBLEND_SRCALPHASAT        = 11,
    D3DBLEND_BOTHSRCALPHA       = 12,
    D3DBLEND_BOTHINVSRCALPHA    = 13,
    D3DBLEND_BLENDFACTOR        = 14,
    D3DBLEND_INVBLENDFACTOR     = 15,
    D3DBLEND_FORCE_DWORD        = 0x7fffffff
} D3DBLEND;

typedef enum _D3DBLENDOP {
    D3DBLENDOP_ADD              = 1,
    D3DBLENDOP_SUBTRACT         = 2,
    D3DBLENDOP_REVSUBTRACT      = 3,
    D3DBLENDOP_MIN              = 4,
    D3DBLENDOP_MAX              = 5,

    D3DBLENDOP_FORCE_DWORD      = 0x7fffffff
} D3DBLENDOP;

typedef struct _D3DVERTEXELEMENT9 {
  WORD    Stream;
  WORD    Offset;
  BYTE    Type;
  BYTE    Method;
  BYTE    Usage;
  BYTE    UsageIndex;
} D3DVERTEXELEMENT9, *LPD3DVERTEXELEMENT9;

#define MAXD3DDECLLENGTH         64 /* +end marker */

#define D3DFVF_RESERVED0           0x0001
#define D3DFVF_POSITION_MASK       0x400E
#define D3DFVF_XYZ                 0x0002
#define D3DFVF_XYZRHW              0x0004
#define D3DFVF_XYZB1               0x0006
#define D3DFVF_XYZB2               0x0008
#define D3DFVF_XYZB3               0x000a
#define D3DFVF_XYZB4               0x000c
#define D3DFVF_XYZB5               0x000e
#define D3DFVF_XYZW                0x4002
#define D3DFVF_NORMAL              0x0010
#define D3DFVF_PSIZE               0x0020
#define D3DFVF_DIFFUSE             0x0040
#define D3DFVF_SPECULAR            0x0080
#define D3DFVF_TEXCOUNT_MASK       0x0f00
#define D3DFVF_TEXCOUNT_SHIFT           8
#define D3DFVF_TEX0                0x0000
#define D3DFVF_TEX1                0x0100
#define D3DFVF_TEX2                0x0200
#define D3DFVF_TEX3                0x0300
#define D3DFVF_TEX4                0x0400
#define D3DFVF_TEX5                0x0500
#define D3DFVF_TEX6                0x0600
#define D3DFVF_TEX7                0x0700
#define D3DFVF_TEX8                0x0800
#define D3DFVF_LASTBETA_UBYTE4     0x1000
#define D3DFVF_LASTBETA_D3DCOLOR   0x8000
#define D3DFVF_RESERVED2           0x6000

typedef enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST             = 1,
    D3DPT_LINELIST              = 2,
    D3DPT_LINESTRIP             = 3,
    D3DPT_TRIANGLELIST          = 4,
    D3DPT_TRIANGLESTRIP         = 5,
    D3DPT_TRIANGLEFAN           = 6,

    D3DPT_FORCE_DWORD           = 0x7fffffff
} D3DPRIMITIVETYPE;

typedef enum _D3DRENDERSTATETYPE {
    D3DRS_ZENABLE                   =   7,
    D3DRS_FILLMODE                  =   8,
    D3DRS_SHADEMODE                 =   9,
    D3DRS_ZWRITEENABLE              =  14,
    D3DRS_ALPHATESTENABLE           =  15,
    D3DRS_LASTPIXEL                 =  16,
    D3DRS_SRCBLEND                  =  19,
    D3DRS_DESTBLEND                 =  20,
    D3DRS_CULLMODE                  =  22,
    D3DRS_ZFUNC                     =  23,
    D3DRS_ALPHAREF                  =  24,
    D3DRS_ALPHAFUNC                 =  25,
    D3DRS_DITHERENABLE              =  26,
    D3DRS_ALPHABLENDENABLE          =  27,
    D3DRS_FOGENABLE                 =  28,
    D3DRS_SPECULARENABLE            =  29,
    D3DRS_FOGCOLOR                  =  34,
    D3DRS_FOGTABLEMODE              =  35,
    D3DRS_FOGSTART                  =  36,
    D3DRS_FOGEND                    =  37,
    D3DRS_FOGDENSITY                =  38,
    D3DRS_RANGEFOGENABLE            =  48,
    D3DRS_STENCILENABLE             =  52,
    D3DRS_STENCILFAIL               =  53,
    D3DRS_STENCILZFAIL              =  54,
    D3DRS_STENCILPASS               =  55,
    D3DRS_STENCILFUNC               =  56,
    D3DRS_STENCILREF                =  57,
    D3DRS_STENCILMASK               =  58,
    D3DRS_STENCILWRITEMASK          =  59,
    D3DRS_TEXTUREFACTOR             =  60,
    D3DRS_WRAP0                     = 128,
    D3DRS_WRAP1                     = 129,
    D3DRS_WRAP2                     = 130,
    D3DRS_WRAP3                     = 131,
    D3DRS_WRAP4                     = 132,
    D3DRS_WRAP5                     = 133,
    D3DRS_WRAP6                     = 134,
    D3DRS_WRAP7                     = 135,
    D3DRS_CLIPPING                  = 136,
    D3DRS_LIGHTING                  = 137,
    D3DRS_AMBIENT                   = 139,
    D3DRS_FOGVERTEXMODE             = 140,
    D3DRS_COLORVERTEX               = 141,
    D3DRS_LOCALVIEWER               = 142,
    D3DRS_NORMALIZENORMALS          = 143,
    D3DRS_DIFFUSEMATERIALSOURCE     = 145,
    D3DRS_SPECULARMATERIALSOURCE    = 146,
    D3DRS_AMBIENTMATERIALSOURCE     = 147,
    D3DRS_EMISSIVEMATERIALSOURCE    = 148,
    D3DRS_VERTEXBLEND               = 151,
    D3DRS_CLIPPLANEENABLE           = 152,
    D3DRS_POINTSIZE                 = 154,
    D3DRS_POINTSIZE_MIN             = 155,
    D3DRS_POINTSPRITEENABLE         = 156,
    D3DRS_POINTSCALEENABLE          = 157,
    D3DRS_POINTSCALE_A              = 158,
    D3DRS_POINTSCALE_B              = 159,
    D3DRS_POINTSCALE_C              = 160,
    D3DRS_MULTISAMPLEANTIALIAS      = 161,
    D3DRS_MULTISAMPLEMASK           = 162,
    D3DRS_PATCHEDGESTYLE            = 163,
    D3DRS_DEBUGMONITORTOKEN         = 165,
    D3DRS_POINTSIZE_MAX             = 166,
    D3DRS_INDEXEDVERTEXBLENDENABLE  = 167,
    D3DRS_COLORWRITEENABLE          = 168,
    D3DRS_TWEENFACTOR               = 170,
    D3DRS_BLENDOP                   = 171,
    D3DRS_POSITIONDEGREE            = 172,
    D3DRS_NORMALDEGREE              = 173,
    D3DRS_SCISSORTESTENABLE         = 174,
    D3DRS_SLOPESCALEDEPTHBIAS       = 175,
    D3DRS_ANTIALIASEDLINEENABLE     = 176,
    D3DRS_MINTESSELLATIONLEVEL      = 178,
    D3DRS_MAXTESSELLATIONLEVEL      = 179,
    D3DRS_ADAPTIVETESS_X            = 180,
    D3DRS_ADAPTIVETESS_Y            = 181,
    D3DRS_ADAPTIVETESS_Z            = 182,
    D3DRS_ADAPTIVETESS_W            = 183,
    D3DRS_ENABLEADAPTIVETESSELLATION= 184,
    D3DRS_TWOSIDEDSTENCILMODE       = 185,
    D3DRS_CCW_STENCILFAIL           = 186,
    D3DRS_CCW_STENCILZFAIL          = 187,
    D3DRS_CCW_STENCILPASS           = 188,
    D3DRS_CCW_STENCILFUNC           = 189,
    D3DRS_COLORWRITEENABLE1         = 190,
    D3DRS_COLORWRITEENABLE2         = 191,
    D3DRS_COLORWRITEENABLE3         = 192,
    D3DRS_BLENDFACTOR               = 193,
    D3DRS_SRGBWRITEENABLE           = 194,
    D3DRS_DEPTHBIAS                 = 195,
    D3DRS_WRAP8                     = 198,
    D3DRS_WRAP9                     = 199,
    D3DRS_WRAP10                    = 200,
    D3DRS_WRAP11                    = 201,
    D3DRS_WRAP12                    = 202,
    D3DRS_WRAP13                    = 203,
    D3DRS_WRAP14                    = 204,
    D3DRS_WRAP15                    = 205,
    D3DRS_SEPARATEALPHABLENDENABLE  = 206,
    D3DRS_SRCBLENDALPHA             = 207,
    D3DRS_DESTBLENDALPHA            = 208,
    D3DRS_BLENDOPALPHA              = 209,

    D3DRS_FORCE_DWORD               = 0x7fffffff
} D3DRENDERSTATETYPE;

/* Macro to deal with LP64 <=> LLP64 differences in numeric constants with 'l' modifier */
#ifndef __MSABI_LONG
# if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
#  define __MSABI_LONG(x)         x ## l
# else
#  define __MSABI_LONG(x)         x
# endif
#endif

#define D3DCOLORWRITEENABLE_RED   (__MSABI_LONG(1)<<0)
#define D3DCOLORWRITEENABLE_GREEN (__MSABI_LONG(1)<<1)
#define D3DCOLORWRITEENABLE_BLUE  (__MSABI_LONG(1)<<2)
#define D3DCOLORWRITEENABLE_ALPHA (__MSABI_LONG(1)<<3)

typedef enum _D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP               =  1,
    D3DTSS_COLORARG1             =  2,
    D3DTSS_COLORARG2             =  3,
    D3DTSS_ALPHAOP               =  4,
    D3DTSS_ALPHAARG1             =  5,
    D3DTSS_ALPHAARG2             =  6,
    D3DTSS_BUMPENVMAT00          =  7,
    D3DTSS_BUMPENVMAT01          =  8,
    D3DTSS_BUMPENVMAT10          =  9,
    D3DTSS_BUMPENVMAT11          = 10,
    D3DTSS_TEXCOORDINDEX         = 11,
    D3DTSS_BUMPENVLSCALE         = 22,
    D3DTSS_BUMPENVLOFFSET        = 23,
    D3DTSS_TEXTURETRANSFORMFLAGS = 24,
    D3DTSS_COLORARG0             = 26,
    D3DTSS_ALPHAARG0             = 27,
    D3DTSS_RESULTARG             = 28,
    D3DTSS_CONSTANT              = 32,

    D3DTSS_FORCE_DWORD           = 0x7fffffff
} D3DTEXTURESTAGESTATETYPE;

typedef enum _D3DTEXTUREOP {
    D3DTOP_DISABLE                   =  1,
    D3DTOP_SELECTARG1                =  2,
    D3DTOP_SELECTARG2                =  3,
    D3DTOP_MODULATE                  =  4,
    D3DTOP_MODULATE2X                =  5,
    D3DTOP_MODULATE4X                =  6,
    D3DTOP_ADD                       =  7,
    D3DTOP_ADDSIGNED                 =  8,
    D3DTOP_ADDSIGNED2X               =  9,
    D3DTOP_SUBTRACT                  = 10,
    D3DTOP_ADDSMOOTH                 = 11,
    D3DTOP_BLENDDIFFUSEALPHA         = 12,
    D3DTOP_BLENDTEXTUREALPHA         = 13,
    D3DTOP_BLENDFACTORALPHA          = 14,
    D3DTOP_BLENDTEXTUREALPHAPM       = 15,
    D3DTOP_BLENDCURRENTALPHA         = 16,
    D3DTOP_PREMODULATE               = 17,
    D3DTOP_MODULATEALPHA_ADDCOLOR    = 18,
    D3DTOP_MODULATECOLOR_ADDALPHA    = 19,
    D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
    D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
    D3DTOP_BUMPENVMAP                = 22,
    D3DTOP_BUMPENVMAPLUMINANCE       = 23,
    D3DTOP_DOTPRODUCT3               = 24,
    D3DTOP_MULTIPLYADD               = 25,
    D3DTOP_LERP                      = 26,

    D3DTOP_FORCE_DWORD               = 0x7fffffff,
} D3DTEXTUREOP;

typedef enum _D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP            = 1,
    D3DTADDRESS_MIRROR          = 2,
    D3DTADDRESS_CLAMP           = 3,
    D3DTADDRESS_BORDER          = 4,
    D3DTADDRESS_MIRRORONCE      = 5,

    D3DTADDRESS_FORCE_DWORD     = 0x7fffffff
} D3DTEXTUREADDRESS;

typedef enum _D3DTEXTUREFILTERTYPE {
    D3DTEXF_NONE            = 0,
    D3DTEXF_POINT           = 1,
    D3DTEXF_LINEAR          = 2,
    D3DTEXF_ANISOTROPIC     = 3,
    D3DTEXF_FLATCUBIC       = 4,
    D3DTEXF_GAUSSIANCUBIC   = 5,
    D3DTEXF_PYRAMIDALQUAD   = 6,
    D3DTEXF_GAUSSIANQUAD    = 7,
    D3DTEXF_FORCE_DWORD     = 0x7fffffff
} D3DTEXTUREFILTERTYPE;

typedef struct _D3DGAMMARAMP {
    WORD                red  [256];
    WORD                green[256];
    WORD                blue [256];
} D3DGAMMARAMP;

typedef enum _D3DSAMPLERSTATETYPE {
    D3DSAMP_ADDRESSU       = 1,
    D3DSAMP_ADDRESSV       = 2,
    D3DSAMP_ADDRESSW       = 3,
    D3DSAMP_BORDERCOLOR    = 4,
    D3DSAMP_MAGFILTER      = 5,
    D3DSAMP_MINFILTER      = 6,
    D3DSAMP_MIPFILTER      = 7,
    D3DSAMP_MIPMAPLODBIAS  = 8,
    D3DSAMP_MAXMIPLEVEL    = 9,
    D3DSAMP_MAXANISOTROPY  = 10,
    D3DSAMP_SRGBTEXTURE    = 11,
    D3DSAMP_ELEMENTINDEX   = 12,
    D3DSAMP_DMAPOFFSET     = 13,

    D3DSAMP_FORCE_DWORD   = 0x7fffffff,
} D3DSAMPLERSTATETYPE;

#define D3DFVF_TEXTUREFORMAT1 3
#define D3DFVF_TEXTUREFORMAT2 0
#define D3DFVF_TEXTUREFORMAT3 1
#define D3DFVF_TEXTUREFORMAT4 2
#define D3DFVF_TEXCOORDSIZE1(CoordIndex) (D3DFVF_TEXTUREFORMAT1 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE2(CoordIndex) (D3DFVF_TEXTUREFORMAT2)
#define D3DFVF_TEXCOORDSIZE3(CoordIndex) (D3DFVF_TEXTUREFORMAT3 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE4(CoordIndex) (D3DFVF_TEXTUREFORMAT4 << (CoordIndex*2 + 16))

typedef enum _D3DDECLMETHOD {
  D3DDECLMETHOD_DEFAULT          = 0,
  D3DDECLMETHOD_PARTIALU         = 1,
  D3DDECLMETHOD_PARTIALV         = 2,
  D3DDECLMETHOD_CROSSUV          = 3,
  D3DDECLMETHOD_UV               = 4,
  D3DDECLMETHOD_LOOKUP           = 5,
  D3DDECLMETHOD_LOOKUPPRESAMPLED = 6
} D3DDECLMETHOD;


#define D3DMAXDECLMETHOD        D3DDECLMETHOD_LOOKUPPRESAMPLED

typedef enum _D3DDECLTYPE {
  D3DDECLTYPE_FLOAT1    =  0,
  D3DDECLTYPE_FLOAT2    =  1,
  D3DDECLTYPE_FLOAT3    =  2,
  D3DDECLTYPE_FLOAT4    =  3,
  D3DDECLTYPE_D3DCOLOR  =  4,
  D3DDECLTYPE_UBYTE4    =  5,
  D3DDECLTYPE_SHORT2    =  6,
  D3DDECLTYPE_SHORT4    =  7,
  /* VS 2.0 */
  D3DDECLTYPE_UBYTE4N   =  8,
  D3DDECLTYPE_SHORT2N   =  9,
  D3DDECLTYPE_SHORT4N   = 10,
  D3DDECLTYPE_USHORT2N  = 11,
  D3DDECLTYPE_USHORT4N  = 12,
  D3DDECLTYPE_UDEC3     = 13,
  D3DDECLTYPE_DEC3N     = 14,
  D3DDECLTYPE_FLOAT16_2 = 15,
  D3DDECLTYPE_FLOAT16_4 = 16,
  D3DDECLTYPE_UNUSED    = 17,
} D3DDECLTYPE;

#define D3DMAXDECLTYPE          D3DDECLTYPE_UNUSED

typedef enum _D3DDECLUSAGE {
  D3DDECLUSAGE_POSITION     = 0,
  D3DDECLUSAGE_BLENDWEIGHT  = 1,
  D3DDECLUSAGE_BLENDINDICES = 2,
  D3DDECLUSAGE_NORMAL       = 3,
  D3DDECLUSAGE_PSIZE        = 4,
  D3DDECLUSAGE_TEXCOORD     = 5,
  D3DDECLUSAGE_TANGENT      = 6,
  D3DDECLUSAGE_BINORMAL     = 7,
  D3DDECLUSAGE_TESSFACTOR   = 8,
  D3DDECLUSAGE_POSITIONT    = 9,
  D3DDECLUSAGE_COLOR        = 10,
  D3DDECLUSAGE_FOG          = 11,
  D3DDECLUSAGE_DEPTH        = 12,
  D3DDECLUSAGE_SAMPLE       = 13
} D3DDECLUSAGE;

typedef enum _D3DFILLMODE {
    D3DFILL_POINT               = 1,
    D3DFILL_WIREFRAME           = 2,
    D3DFILL_SOLID               = 3,

    D3DFILL_FORCE_DWORD         = 0x7fffffff
} D3DFILLMODE;

typedef enum _D3DLIGHTTYPE {
    D3DLIGHT_POINT          = 1,
    D3DLIGHT_SPOT           = 2,
    D3DLIGHT_DIRECTIONAL    = 3,

    D3DLIGHT_FORCE_DWORD    = 0x7fffffff
} D3DLIGHTTYPE;

#ifndef D3DCOLOR_DEFINED
typedef DWORD D3DCOLOR;
#define D3DCOLOR_DEFINED
#endif

typedef enum _D3DSHADEMODE {
    D3DSHADE_FLAT               = 1,
    D3DSHADE_GOURAUD            = 2,
    D3DSHADE_PHONG              = 3,

    D3DSHADE_FORCE_DWORD        = 0x7fffffff
} D3DSHADEMODE;

typedef enum _D3DSWAPEFFECT {
    D3DSWAPEFFECT_DISCARD         = 1,
    D3DSWAPEFFECT_FLIP            = 2,
    D3DSWAPEFFECT_COPY            = 3,
    D3DSWAPEFFECT_OVERLAY         = 4,
    D3DSWAPEFFECT_FLIPEX          = 5,
    D3DSWAPEFFECT_FORCE_DWORD     = 0xFFFFFFFF
} D3DSWAPEFFECT;

typedef enum _D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW            =  2,
    D3DTS_PROJECTION      =  3,
    D3DTS_TEXTURE0        = 16,
    D3DTS_TEXTURE1        = 17,
    D3DTS_TEXTURE2        = 18,
    D3DTS_TEXTURE3        = 19,
    D3DTS_TEXTURE4        = 20,
    D3DTS_TEXTURE5        = 21,
    D3DTS_TEXTURE6        = 22,
    D3DTS_TEXTURE7        = 23,

    D3DTS_FORCE_DWORD     = 0x7fffffff
} D3DTRANSFORMSTATETYPE;

typedef enum _D3DTEXTURETRANSFORMFLAGS {
    D3DTTFF_DISABLE         =   0,
    D3DTTFF_COUNT1          =   1,
    D3DTTFF_COUNT2          =   2,
    D3DTTFF_COUNT3          =   3,
    D3DTTFF_COUNT4          =   4,
    D3DTTFF_PROJECTED       = 256,

    D3DTTFF_FORCE_DWORD     = 0x7fffffff
} D3DTEXTURETRANSFORMFLAGS;

#define D3DTS_WORLD  D3DTS_WORLDMATRIX(0)
#define D3DTS_WORLD1 D3DTS_WORLDMATRIX(1)
#define D3DTS_WORLD2 D3DTS_WORLDMATRIX(2)
#define D3DTS_WORLD3 D3DTS_WORLDMATRIX(3)
#define D3DTS_WORLDMATRIX(index) (D3DTRANSFORMSTATETYPE)(index + 256)

#define D3DUSAGE_RENDERTARGET       __MSABI_LONG(0x00000001)
#define D3DUSAGE_DEPTHSTENCIL       __MSABI_LONG(0x00000002)
#define D3DUSAGE_WRITEONLY          __MSABI_LONG(0x00000008)
#define D3DUSAGE_SOFTWAREPROCESSING __MSABI_LONG(0x00000010)
#define D3DUSAGE_DONOTCLIP          __MSABI_LONG(0x00000020)
#define D3DUSAGE_POINTS             __MSABI_LONG(0x00000040)
#define D3DUSAGE_RTPATCHES          __MSABI_LONG(0x00000080)
#define D3DUSAGE_NPATCHES           __MSABI_LONG(0x00000100)
#define D3DUSAGE_DYNAMIC            __MSABI_LONG(0x00000200)
#define D3DUSAGE_AUTOGENMIPMAP      __MSABI_LONG(0x00000400)
#define D3DUSAGE_DMAP               __MSABI_LONG(0x00004000)

#define D3DCOLOR_ARGB(a,r,g,b)       ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_COLORVALUE(r,g,b,a) D3DCOLOR_RGBA((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f),(DWORD)((a)*255.f))
#define D3DCOLOR_RGBA(r,g,b,a)       D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_XRGB(r,g,b)         D3DCOLOR_ARGB(0xff,r,g,b)
#define D3DCOLOR_XYUV(y,u,v)         D3DCOLOR_ARGB(0xFF,y,u,v)
#define D3DCOLOR_AYUV(a,y,u,v)       D3DCOLOR_ARGB(a,y,u,v)

#define D3DDECL_END() {0xFF,0,D3DDECLTYPE_UNUSED,0,0,0}

#ifndef D3DRECT_DEFINED
typedef struct _D3DRECT {
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
} D3DRECT;
#define D3DRECT_DEFINED
#endif

typedef DWORD           FOURCC;

typedef struct _AVIINDEXENTRY {
    DWORD	ckid;
    DWORD	dwFlags;
    DWORD	dwChunkOffset;
    DWORD	dwChunkLength;
} AVIINDEXENTRY;

typedef void *HIC;

#define D3DTA_SELECTMASK        0x0000000f
#define D3DTA_DIFFUSE           0x00000000
#define D3DTA_CURRENT           0x00000001
#define D3DTA_TEXTURE           0x00000002
#define D3DTA_TFACTOR           0x00000003
#define D3DTA_SPECULAR          0x00000004
#define D3DTA_TEMP              0x00000005
#define D3DTA_CONSTANT          0x00000006
#define D3DTA_COMPLEMENT        0x00000010
#define D3DTA_ALPHAREPLICATE    0x00000020

#define D3DTSS_TCI_PASSTHRU                       0x00000
#define D3DTSS_TCI_CAMERASPACENORMAL              0x10000
#define D3DTSS_TCI_CAMERASPACEPOSITION            0x20000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR    0x30000
#define D3DTSS_TCI_SPHEREMAP                      0x40000

inline BOOL SwitchToThread() { return (0 == sched_yield()); }

#define xr_fs_strlwr(str) str
#define xr_fs_nostrlwr(str) xr_strlwr(str)

/// For backward compability of FS, for real filesystem delimiter set to back
inline void restore_path_separators(char * path)
{
    while (char* sep = strchr(path, '/')) *sep = '\\'; //
}

inline void convert_path_separators(char * path)
{
    while (char* sep = strchr(path, '\\')) *sep = '/';
}

#define xr_unlink unlink

inline tm* localtime_safe(const time_t *time, struct tm* result){ return localtime_r(time, result); }

#define xr_strerror(errno, buffer, bufferSize) strerror_r(errno, buffer, sizeof(buffer))

using xrpid_t = pid_t;
