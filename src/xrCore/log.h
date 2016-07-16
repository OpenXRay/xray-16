#pragma once
#ifndef logH
#define logH
#include "Common/Platform.hpp"
#include "xrCore_impexp.h"

class shared_str;
template <typename T, typename allocator> class xr_vector;

#define VPUSH(a) ((a).x), ((a).y), ((a).z)

void XRCORE_API __cdecl Msg(LPCSTR format, ...);
void XRCORE_API Log(LPCSTR msg);
void XRCORE_API Log(LPCSTR msg);
void XRCORE_API Log(LPCSTR msg, LPCSTR dop);
void XRCORE_API Log(LPCSTR msg, u32 dop);
void XRCORE_API Log(LPCSTR msg, int dop);
void XRCORE_API Log(LPCSTR msg, float dop);
void XRCORE_API Log(LPCSTR msg, const Fvector& dop);
void XRCORE_API Log(LPCSTR msg, const Fmatrix& dop);
void XRCORE_API LogWinErr(LPCSTR msg, long err_code);

struct LogCallback
{
    typedef void (*Func)(void* context, const char* s);
    Func Log;
    void* Context;

    LogCallback() : Log(nullptr), Context(nullptr) {}
    LogCallback(nullptr_t) : Log(nullptr), Context(nullptr) {}
    LogCallback(Func log, void* ctx) : Log(log), Context(ctx) {}
    void operator()(const char* s) { Log(Context, s); }
    operator bool() const { return !!Log; }
};

LogCallback XRCORE_API SetLogCB(const LogCallback& cb);
void XRCORE_API CreateLog(BOOL no_log = FALSE);
void InitLog();
void CloseLog();
void XRCORE_API FlushLog();

extern XRCORE_API xr_vector<shared_str>* LogFile;
extern XRCORE_API BOOL LogExecCB;

#endif
