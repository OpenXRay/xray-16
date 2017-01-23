#pragma once
#include "xrCore/xrCore.h"
#include "xrScriptEngine/xrScriptEngine.hpp"

class CScriptDebugger;

struct SPath
{
    string_path path;
};

class XRSCRIPTENGINE_API CScriptCallStack
{
public:
    CScriptDebugger *m_debugger;
    void GotoStackTraceLevel(int nLevel);
    void Add(const char *szDesc, const char *szFile, int nLine);
    void Clear();
    CScriptCallStack(CScriptDebugger *d);
    ~CScriptCallStack();

    int GetLevel() { return m_nCurrentLevel; }

    void SetStackTraceLevel(int);
protected:
    int m_nCurrentLevel;
    xr_vector<u32> m_levels;
    xr_vector<u32> m_lines;
    xr_vector<SPath> m_files;
};
