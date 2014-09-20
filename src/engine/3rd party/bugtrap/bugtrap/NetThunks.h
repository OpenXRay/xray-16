/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Managed to unmanaged code thunks.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#ifdef _MANAGED

#include "Encoding.h"
#include "SymEngineNet.h"

using namespace IntelleSoft::BugTrap;

class CNetStackTrace
{
public:
	struct CNetStackTraceEntry
	{
		CNetStackTraceEntry(void);
		WCHAR m_szAssembly[MAX_PATH];
		WCHAR m_szILOffset[32];
		WCHAR m_szNativeOffset[32];
		WCHAR m_szType[256];
		WCHAR m_szMethod[256];
		WCHAR m_szMethodSignature[256];
		WCHAR m_szSourceFile[MAX_PATH];
		WCHAR m_szLineNumber[16];
		WCHAR m_szColumnNumber[16];
		WCHAR m_szLineInfo[64];
	};

	struct CNetErrorInfo : public CNetStackTraceEntry
	{
		CNetErrorInfo(void);
		WCHAR m_szException[128];
		WCHAR m_szMessage[256];
		WCHAR m_szAppDomainName[MAX_PATH];
		WCHAR m_szAppDomainID[16];
		WCHAR m_szProcessName[MAX_PATH];
		WCHAR m_szProcessID[16];
	};

	CNetStackTrace(void);
	CNetStackTrace(gcroot<Thread^> thread);
	void InitStackTrace(void);
	bool GetFirstStackTraceEntry(CNetStackTraceEntry& rEntry);
	bool GetNextStackTraceEntry(CNetStackTraceEntry& rEntry);
	bool GetFirstStackTraceString(CUTF8EncStream& rEncStream);
	bool GetNextStackTraceString(CUTF8EncStream& rEncStream);
	bool GetErrorInfo(CNetErrorInfo& rErrorInfo);
	void GetErrorString(CStrStream& rStream);
	void GetErrorString(CUTF8EncStream& rEncStream);

private:
	CNetStackTrace(const CNetStackTrace& rStackTrace);
	CNetStackTrace& operator=(const CNetStackTrace& rStackTrace);
	void GetStackTraceString(const CNetStackTraceEntry& rEntry, CUTF8EncStream& rEncStream);

	gcroot<StackFrameEnumerator^> m_gcStackFrameEnumerator;
};

class CNetAssemblies
{
public:
	struct CAssemblyInfo
	{
		CAssemblyInfo(void);
		WCHAR m_szName[MAX_PATH];
		WCHAR m_szVersion[64];
		WCHAR m_szFileVersion[64];
		WCHAR m_szCodeBase[MAX_PATH];
	};

	CNetAssemblies(void);
	void InitAssemblies(void);
	bool GetFirstAssembly(CNetAssemblies::CAssemblyInfo& rAssemblyInfo);
	bool GetNextAssembly(CNetAssemblies::CAssemblyInfo& rAssemblyInfo);

private:
	gcroot<AssemblyEnumerator^> m_gcAssemblyEnumerator;
};

#pragma managed /////////////////////////////////////////////////////////////////////////////////

namespace NetThunks
{

	inline bool IsNetException(void)
	{
		return (ExceptionHandler::Exception != nullptr);
	}

	void GetThreadInfo(gcroot<Thread^> gcThread, DWORD& dwThreadID, PWSTR pszThreadName, DWORD dwThreadNameSize);

	void GetAppDomainInfo(DWORD& dwAppDomainID, PWSTR pszAppDomainName, DWORD dwAppDomainSize);

	inline void GetThreadInfo(DWORD& dwThreadID, PWSTR pszThreadName, DWORD dwThreadNameSize)
	{
		GetThreadInfo(Thread::CurrentThread, dwThreadID, pszThreadName, dwThreadNameSize);
	}

	gcroot<StackFrameEnumerator^> EnumStackFrames(void);

	gcroot<StackFrameEnumerator^> EnumStackFrames(gcroot<Thread^> thread);

	void InitStackTrace(gcroot<StackFrameEnumerator^> gcStackFrameEnumerator);

	bool GetFirstStackTraceEntry(gcroot<StackFrameEnumerator^> gcStackFrameEnumerator, CNetStackTrace::CNetStackTraceEntry& rEntry);

	bool GetNextStackTraceEntry(gcroot<StackFrameEnumerator^> gcStackFrameEnumerator, CNetStackTrace::CNetStackTraceEntry& rEntry);

	bool GetErrorInfo(gcroot<StackFrameEnumerator^> gcStackFrameEnumerator, CNetStackTrace::CNetErrorInfo& rErrorInfo);

	void GetNetVersion(PWSTR pszNetVersion, DWORD dwNetVersionSize);

	gcroot<AssemblyEnumerator^> EnumAssemblies(void);

	void InitAssemblies(gcroot<AssemblyEnumerator^> gcAssemblyEnumerator);

	bool GetFirstAssembly(gcroot<AssemblyEnumerator^> gcAssemblyEnumerator, CNetAssemblies::CAssemblyInfo& rAssemblyInfo);

	bool GetNextAssembly(gcroot<AssemblyEnumerator^> gcAssemblyEnumerator, CNetAssemblies::CAssemblyInfo& rAssemblyInfo);

	bool ReadVersionInfo(void);

	void FireBeforeUnhandledExceptionEvent(void);

	void FireAfterUnhandledExceptionEvent(void);

	void FlushTraceListeners(void);

	gcroot<Thread^> GetCurrentThread(void);

}

#pragma unmanaged ///////////////////////////////////////////////////////////////////////////////

inline CNetStackTrace::CNetStackTrace(void) :
	m_gcStackFrameEnumerator(NetThunks::EnumStackFrames())
{
}

inline CNetStackTrace::CNetStackTrace(gcroot<Thread^> thread) :
	m_gcStackFrameEnumerator(NetThunks::EnumStackFrames(thread))
{
}

inline void CNetStackTrace::InitStackTrace(void)
{
	NetThunks::InitStackTrace(m_gcStackFrameEnumerator);
}

inline bool CNetStackTrace::GetFirstStackTraceEntry(CNetStackTraceEntry& rEntry)
{
	return NetThunks::GetFirstStackTraceEntry(m_gcStackFrameEnumerator, rEntry);
}

inline bool CNetStackTrace::GetNextStackTraceEntry(CNetStackTraceEntry& rEntry)
{
	return NetThunks::GetNextStackTraceEntry(m_gcStackFrameEnumerator, rEntry);
}

inline bool CNetStackTrace::GetErrorInfo(CNetErrorInfo& rErrorInfo)
{
	return NetThunks::GetErrorInfo(m_gcStackFrameEnumerator, rErrorInfo);
}

inline CNetAssemblies::CNetAssemblies(void) :
	m_gcAssemblyEnumerator(NetThunks::EnumAssemblies())
{
}

inline void CNetAssemblies::InitAssemblies(void)
{
	NetThunks::InitAssemblies(m_gcAssemblyEnumerator);
}

inline bool CNetAssemblies::GetFirstAssembly(CNetAssemblies::CAssemblyInfo& rAssemblyInfo)
{
	return NetThunks::GetFirstAssembly(m_gcAssemblyEnumerator, rAssemblyInfo);
}

inline bool CNetAssemblies::GetNextAssembly(CNetAssemblies::CAssemblyInfo& rAssemblyInfo)
{
	return NetThunks::GetNextAssembly(m_gcAssemblyEnumerator, rAssemblyInfo);
}

#endif // _MANAGED
