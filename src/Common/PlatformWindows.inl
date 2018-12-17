#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define IDIRECTPLAY2_OR_GREATER // ?

#ifndef _WIN32_WINNT
// Request Windows 7 functionality
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif

#include <sys/utime.h>

#define NOGDICAPMASKS
//#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NODRAWTEXT
#define NOMEMMGR
#define NOMETAFILE
#define NOSERVICE
#define NOCOMM
#define NOHELP
#define NOPROFILER
#define NOMCX

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define DOSWIN32
#define _WIN32_DCOM

#include <windows.h>
#include <windowsx.h>
#include <ctime>

inline void convert_path_separators(char * path) {}
inline void restore_path_separators(char * path) {}

inline tm* localtime_safe(const time_t *time, struct tm* result){ return localtime_s(result, time) == 0 ? result : NULL; }

#define tid_t DWORD
