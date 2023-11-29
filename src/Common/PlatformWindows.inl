#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#ifndef _WIN32_WINNT
// Request Windows 7 functionality
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
//#define NOWINMESSAGES
//#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
//#define NORASTEROPS
//#define NOSHOWWINDOW
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
//#define NOCTLMGR
#define NODRAWTEXT
//#define NOGDI
#define NOKERNEL
//#define NOUSER
//#define NONLS
//#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
//#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
//#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOCRYPT
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NOIME

#define DOSWIN32
#define _WIN32_DCOM

#include <windows.h>
#include <ctime>

#define xr_fs_strlwr(str) xr_strlwr(str)
#define xr_fs_nostrlwr(str) str

inline void restore_path_separators(char* /*path*/) {}
inline void convert_path_separators(char* /*path*/) {}

inline tm* localtime_safe(const time_t *time, struct tm* result){ return localtime_s(result, time) == 0 ? result : NULL; }

#define xr_strerror(errno, buffer, bufferSize) strerror_s(buffer, sizeof(buffer), errno)

using xrpid_t = DWORD;

using ssize_t = SSIZE_T;
