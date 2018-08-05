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
#include <algorithm> // for min max

#include <string>
#include <alloca.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h> // for mmap / munmap
#include <dirent.h>
#include <utime.h>

#include <SDL2/SDL.h>
#define CONFIG_USE_SDL

#define _LINUX // for GameSpy

#if !defined(__INTEL_COMPILER)
#define _alloca alloca
#endif

#define _MAX_PATH PATH_MAX + 1
#define MAX_PATH PATH_MAX + 1

#define WINAPI

#define _copysign copysign

#define _cdecl //__attribute__((cdecl))
#define _stdcall //__attribute__((stdcall))
#define _fastcall //__attribute__((fastcall))

#define __cdecl
#define __stdcall
#define __fastcall

//#define __declspec
#define __forceinline FORCE_INLINE
#define __pragma(...) _Pragma(#__VA_ARGS__)
#define __declspec(x)
#define CALLBACK
#define TEXT(x) strdup(x)

inline char* _strlwr_l(char* str, locale_t loc)
{
//TODO
}

inline char* _strupr_l(char* str, locale_t loc)
{
//TODO
}

#define VOID void
#define HKL void*
#define ActivateKeyboardLayout(x, y) {}
#define ScreenToClient(hwnd, p) {}

#define __except(X) catch(X)

/*
static inline long InterlockedExchange(volatile long* val, long new_val)
{
  long old_val;
  do {
    old_val = *val;
  } while (__sync_val_compare_and_swap (val, old_val, new_val) != old_val);
    return old_val;
}
*/

inline pthread_t GetCurrentThreadId()
{
    return pthread_self();
}

inline void Sleep(int ms)
{
    SDL_Delay(ms);
}

inline void _splitpath (
        const char* path,  // Path Input
        char* drive,       // Drive     : Output
        char* dir,         // Directory : Output
        char* fname,       // Filename  : Output
        char* ext          // Extension : Output
){}

inline unsigned long GetLastError()
{
    return 0;
}

inline int GetExceptionCode()
{
    return 0;
}

#define xr_unlink unlink

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
typedef unsigned long& ULONG_PTR;
typedef long long int LARGE_INTEGER;
typedef unsigned long long int ULARGE_INTEGER;

typedef unsigned short* LPWORD;
typedef unsigned long* LPDWORD;
typedef const void* LPCVOID;
typedef long long int* PLARGE_INTEGER;

typedef wchar_t WCHAR;

#define WAVE_FORMAT_PCM  0x0001

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

#ifdef XR_X64
typedef int64_t INT_PTR;
typedef uint16_t UINT_PTR;
typedef int64_t LONG_PTR;
#else
typedef int INT_PTR;
typedef unsigned int UINT_PTR;
typedef long LONG_PTR;
#endif // XR_X64

typedef int HANDLE;
typedef void* HMODULE;
typedef void* LPVOID;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef long HRESULT;
typedef long LRESULT;
typedef long _W64;
//typedef void* HWND;
typedef SDL_Window* HWND;
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
#define WA_INACTIVE 0
#define HIWORD(l)              ((WORD)((DWORD_PTR)(l) >> 16))
#define LOWORD(l)              ((WORD)((DWORD_PTR)(l) & 0xFFFF))


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

#define _stricmp stricmp
#define strcmpi stricmp
#define lstrcpy strcpy
#define stricmp strcasecmp
#define strupr SDL_strupr
#define strncpy_s(dest, size, source, num) (NULL == strncpy(dest, source, num))
#define strcpy_s(dest, num, source) (NULL == strcpy(dest, source))
#define strcat_s(dest, num, source) (dest == strcat(dest, source))
#define _vsnprintf vsnprintf
#define vsprintf_s(dest, size, format, args) vsprintf(dest, format, args)
#define _alloca alloca
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
#define _mkdir(dir) mkdir(dir, S_IRWXU)
#define _wtoi(arg) wcstol(arg, NULL, 10)
#define _wtoi64(arg) wcstoll(arg, NULL, 10)
#undef min
#undef max
#define __max(a, b) std::max(a, b)
#define __min(a, b) std::min(a, b)

#define itoa SDL_itoa
#define _itoa_s(value, buffer, radix) SDL_itoa(value, buffer, radix)
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
