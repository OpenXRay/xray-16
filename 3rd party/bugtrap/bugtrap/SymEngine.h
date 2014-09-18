/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Low-level wrapper for Debug Help API.
 * Author: Maksim Pyatkovskiy.
 * Note: Portions of this code are based on Bugslayer code developed by John Robbins.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "EnumProcess.h"
#include "StrStream.h"
#include "Encoding.h"
#include "XmlWriter.h"
#include "SmartPtr.h"
#include "InterfacePtr.h"

#ifdef _MANAGED
#include "NetThunks.h"
#endif

/// Type definition of pointer to SymGetOptions() function.
typedef DWORD (WINAPI *PFSymGetOptions)(VOID);
/// Type definition of pointer to SymSetOptions() function.
typedef DWORD (WINAPI *PFSymSetOptions)(DWORD SymOptions);
/// Type definition of pointer to SymInitialize() function.
typedef BOOL (WINAPI *PFSymInitialize)(HANDLE hProcess, PSTR UserSearchPath, BOOL fInvadeProcess);
/// Type definition of pointer to SymCleanup() function.
typedef BOOL (WINAPI *PFSymCleanup)(HANDLE hProcess);
/// Type definition of pointer to SymGetModuleBase64() function.
typedef DWORD64 (WINAPI *PFSymGetModuleBase64)(HANDLE hProcess, DWORD64 dwAddr);
/// Type definition of pointer to SymFromAddr() function.
typedef BOOL (WINAPI *PFSymFromAddr)(HANDLE hProcess, DWORD64 dwAddr, PDWORD64 pdwDisplacement, PSYMBOL_INFO Symbol);
/// Type definition of pointer to SymGetLineFromAddr64() function.
typedef BOOL (WINAPI *PFSymGetLineFromAddr64)(HANDLE hProcess, DWORD64 dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line);
/// Type definition of pointer to StackWalk64() function.
typedef BOOL (WINAPI *PFStackWalk64)(DWORD dwMachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
/// Type definition of pointer to SymFunctionTableAccess64() function.
typedef PVOID (WINAPI *PFSymFunctionTableAccess64)(HANDLE hProcess, DWORD64 AddrBase);
/// Type definition of pointer to MiniDumpWriteDump() function.
typedef BOOL (WINAPI *PFMiniDumpWriteDump)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
/// Opens an existing thread object.
typedef HANDLE (WINAPI *PFOpenThread)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId);

/// Low-level wrapper for Debug Help API.
class CSymEngine
{
public:
	/// Type of exception.
	enum EXCEPTION_TYPE
	{
		/// Not an exception.
		NO_EXCEPTION,
		/// Win32 exception.
		WIN32_EXCEPTION,
		/// C++ exception.
		CPP_EXCEPTION,
#ifdef _MANAGED
		/// .NET exception.
		NET_EXCEPTION
#endif
	};

	/// Screen-shot object.
	class CScreenShot
	{
	public:
		/// Create the object.
		CScreenShot(void);
		/// Destroy the object.
		~CScreenShot(void);
		/// Write screen-shot to file.
		BOOL WriteScreenShot(PCTSTR pszFileName);

	private:
		/// Protects the class from being accidentally copied.
		CScreenShot(const CScreenShot& rScreenShot);
		/// Protects the class from being accidentally copied.
		CScreenShot& operator=(const CScreenShot& rScreenShot);

		/// Bitmap information.
		PBITMAPINFO m_pBmpInfo;
		/// Bitmap bits.
		PBYTE m_pBitsArray;
		/// Bitmap header size.
		DWORD m_dwBmpHdrSize;
		/// Image size.
		DWORD m_dwBitsArraySize;
	};

	/// Smart pointer to the screen-shot.
	typedef CSmartPtr<CScreenShot, CPtrLinkEngine<CScreenShot, CPtrTraits> > CScreenShotPtr;

	/// Symbolic engine parameters.
	class CEngineParams
	{
	public:
		/// Initialize member variables.
		CEngineParams(void);
		/// Initialize member variables.
		CEngineParams(PEXCEPTION_POINTERS pExceptionPointers, EXCEPTION_TYPE eExceptionType);

		/// Current exception information.
		PEXCEPTION_POINTERS m_pExceptionPointers;
		/// COM error information.
		CInterfacePtr<IErrorInfo> m_pErrorInfo;
		/// System error code.
		DWORD m_dwLastError;
		/// Type of the exception.
		EXCEPTION_TYPE m_eExceptionType;
		/// Date and time of the crash.
		SYSTEMTIME m_DateTime;
		/// Screen shot data.
		CScreenShotPtr m_pScreenShot;

	private:
		/// Make a screen shot.
		void MakeScreenShot(void);
	};

	/// Stack entry information.
	struct CStackTraceEntry
	{
		/// Initialize data members.
		CStackTraceEntry(void);
		/// Module name that contains the entry.
		TCHAR m_szModule[MAX_PATH];
		/// Address of еру entry.
		TCHAR m_szAddress[32];
		/// Function where еру entry is located.
		TCHAR m_szFunctionName[512];
		/// Byte offset from the beginning of the function.
		TCHAR m_szFunctionOffset[16];
		/// User-friendly function name and offset.
		TCHAR m_szFunctionInfo[512];
		/// Name of source file that contains the entry.
		TCHAR m_szSourceFile[MAX_PATH];
		/// Number of line in source file.
		TCHAR m_szLineNumber[16];
		/// Byte offset from the beginning of the line.
		TCHAR m_szLineOffset[16];
		/// User-friendly line number and offset.
		TCHAR m_szLineInfo[64];
	};

private:
	/// Context record of stack walk operations.
	struct CStackWalkContext
	{
		/// Initialize the object.
		CStackWalkContext(void);
		/// Current stack frame information used by stack tracing.
		STACKFRAME64 m_stFrame;
		/// Current context information used by stack tracing.
		CONTEXT m_context;
		/// Handle of examined thread.
		HANDLE m_hThread;
	};

	/// Operating system information.
	struct COsInfo
	{
		/// Initialize the object.
		COsInfo(void);
		/// Windows version.
		PCTSTR m_pszWinVersion;
		/// Service pack version.
		TCHAR m_szSPVersion[128];
		/// Build number.
		TCHAR m_szBuildNumber[16];
#ifdef _MANAGED
		/// CLR Version.
		WCHAR m_szNetVersion[64];
#endif
	};

	/// Error information.
	struct CErrorInfo : public CStackTraceEntry
	{
		/// Initialize the object.
		CErrorInfo(void);
		/// What happened?
		PCTSTR m_pszWhat;
		/// Process file name.
		TCHAR m_szProcessName[MAX_PATH];
		/// Process ID.
		TCHAR m_szProcessID[16];
	};

	/// CPUs information.
	struct CCpusInfo
	{
		/// Initialize the object.
		CCpusInfo(void);
		/// Processors architecture.
		PCTSTR m_pszCpuArch;
		/// Number of processors.
		DWORD m_dwNumCpus;
	};

	/// CPU information.
	struct CCpuInfo
	{
		/// Initialize the object.
		CCpuInfo(void);
		/// Processor identifier.
		TCHAR m_szCpuId[128];
		/// Processor speed.
		TCHAR m_szCpuSpeed[16];
		/// Processor description.
		TCHAR m_szCpuDescription[128];
	};

	/// Memory information.
	struct CMemInfo
	{
		/// Initialize the object.
		CMemInfo(void);
		/// Current memory load
		TCHAR m_szMemoryLoad[8];
		/// Total physical memory
		TCHAR m_szTotalPhys[16];
		/// Available physical memory
		TCHAR m_szAvailPhys[16];
		/// Total page file memory
		TCHAR m_szTotalPageFile[16];
		/// Available page file memory
		TCHAR m_szAvailPageFile[16];
	};

	/// System error information.
	struct CSysErrorInfo
	{
	public:
		/// Initialize the object.
		CSysErrorInfo(void);
		/// De-initialize the object.
		~CSysErrorInfo(void);
		/// Copy the object.
		CSysErrorInfo(const CSysErrorInfo& rErrorInfo);
		/// Copy the object.
		CSysErrorInfo& operator=(const CSysErrorInfo& rErrorInfo);
		/// Last error code.
		TCHAR m_szErrorCode[16];
		/// Error description.
		PTSTR m_pszMessageBuffer;

	private:
		/// Free member variables.
		void FreeData(void);
		/// Copy member variables.
		void CopyData(const CSysErrorInfo& rErrorInfo);
	};

	/// COM error information.
	struct CComErrorInfo
	{
		/// A textual description of the error.
		CStrHolder m_strDescription;
		/// The path of the help file that describes the error.
		CStrHolder m_strHelpFile;
		/// The programmatic identifier (ProgID) for the class or application that returned the error.
		CStrHolder m_strSource;
		/// The globally unique identifier (GUID) for the interface that defined the error.
		CStrHolder m_strGuid;
	};

	/// Keeps registers values in string format.
	struct CRegistersValues
	{
		/// Initialize the object.
		CRegistersValues(void);
		/// EAX register.
		TCHAR m_szEax[16];
		/// EBX register.
		TCHAR m_szEbx[16];
		/// ECX register.
		TCHAR m_szEcx[16];
		/// EDX register.
		TCHAR m_szEdx[16];
		/// ESI register.
		TCHAR m_szEsi[16];
		/// EDI register.
		TCHAR m_szEdi[16];
		/// ESP register.
		TCHAR m_szEsp[16];
		/// EBP register.
		TCHAR m_szEbp[16];
		/// EIP register.
		TCHAR m_szEip[16];
		/// CS register.
		TCHAR m_szSegCs[16];
		/// DS register.
		TCHAR m_szSegDs[16];
		/// SS register.
		TCHAR m_szSegSs[16];
		/// ES register.
		TCHAR m_szSegEs[16];
		/// FS register.
		TCHAR m_szSegFs[16];
		/// GS register.
		TCHAR m_szSegGs[16];
		/// EFLAGS register.
		TCHAR m_szEFlags[16];
	};

	/// Handle to the process for which symbols are to be maintained.
	HANDLE m_hSymProcess;
	/// Handle of DbgHelp.dll module.
	HINSTANCE m_hDbgHelpDll;
	/// Handle of Kernel32.dll module.
	HINSTANCE m_hKernelDll;
	/// Current exception information.
	PEXCEPTION_POINTERS m_pExceptionPointers;
	/// Date and time of the crash.
	SYSTEMTIME m_DateTime;
	/// Type of the exception.
	EXCEPTION_TYPE m_eExceptionType;
	/// Exception address corrected according to C++ exception information.
	DWORD64 m_dwExceptionAddress;
	/// Start context exception information.
	CONTEXT m_StartExceptionContext;
	/// COM error information.
	CInterfacePtr<IErrorInfo> m_pErrorInfo;
	/// System error code.
	DWORD m_dwLastError;
	/// Screen shot data.
	CScreenShotPtr m_pScreenShot;
	/// Stack walk context record.
	CStackWalkContext m_swContext;
	/// Frame count (in rare cases DbgHelp could produce infinite stack traces).
	DWORD m_dwFrameCount;

#ifdef _MANAGED
	/// Managed stack trace.
	CNetStackTrace* m_pNetStackTrace;
#endif

	/// Pointer to SymGetOptions() function.
	PFSymGetOptions FSymGetOptions;
	/// Pointer to SymSetOptions() function.
	PFSymSetOptions FSymSetOptions;
	/// Pointer to SymInitialize() function.
	PFSymInitialize FSymInitialize;
	/// Pointer to SymCleanup() function.
	PFSymCleanup FSymCleanup;
	/// Pointer to SymGetModuleBase64() function.
	PFSymGetModuleBase64 FSymGetModuleBase64;
	/// Pointer to SymFromAddr() function.
	PFSymFromAddr FSymFromAddr;
	/// Pointer to SymGetLineFromAddr64() function.
	PFSymGetLineFromAddr64 FSymGetLineFromAddr64;
	/// Pointer to StackWalk64() function.
	PFStackWalk64 FStackWalk64;
	/// Pointer to SymFunctionTableAccess64() function.
	PFSymFunctionTableAccess64 FSymFunctionTableAccess64;
	/// Pointer to MiniDumpWriteDump() function.
	PFMiniDumpWriteDump FMiniDumpWriteDump;
	/// Pointer to OpenThread() function.
	PFOpenThread FOpenThread;

	/// Get string representation of system exception code.
	static PCTSTR ConvertExceptionCodeToString(DWORD dwException);
	/// Adapter for ReadProcessMemory() function called from StackWalk64() function.
	static BOOL CALLBACK ReadProcessMemoryProc64(HANDLE hProcess, DWORD64 pBaseAddress, PVOID pBuffer, DWORD nSize, PDWORD pNumberOfBytesRead);
	/// Safely copy memory blocks.
	static void SafeCopy(PVOID pDestination, PVOID pSource, DWORD dwSize);
	/// Add new file to zip archive.
	static BOOL AddFileToArchive(zipFile hZipFile, PCTSTR pszFilePath, PCTSTR pszFileName);
	/// Adjust exception stack frame according to C++ exception.
	BOOL AdjustExceptionStackFrame(void);
	/// Set stack frame structure to initial value.
	BOOL InitStackTrace(HANDLE hThread);
	/// Set stack frame structure to initial value.
	BOOL InitStackTrace(LPSTACKFRAME64 pStackFrame);
	/// Get stack trace info for the interrupted thread.
	void GetWin32StackTrace(CUTF8EncStream& rEncStream, DWORD dwThreadID, HANDLE hThread, PCSTR pszThreadStatus);
	/// Get stack trace info for the interrupted thread.
	void GetWin32StackTrace(CXmlWriter& rXmlWriter, DWORD dwThreadID, HANDLE hThread, PCTSTR pszThreadStatus);
	/// Get error reason information.
	void GetErrorReason(CXmlWriter& rXmlWriter);
	/// Get register values.
	void GetRegistersInfo(CXmlWriter& rXmlWriter);
	/// Get system error information.
	void GetSysErrorInfo(CXmlWriter& rXmlWriter);
	/// Get COM error information.
	void GetComErrorInfo(CXmlWriter& rXmlWriter);
	/// Get description of system CPUs.
	void GetCpusInfo(CXmlWriter& rXmlWriter);
	/// Get OS information.
	void GetOsInfo(CXmlWriter& rXmlWriter);
	/// Get system memory information.
	void GetMemInfo(CXmlWriter& rXmlWriter);
#ifdef _MANAGED
	/// Get stack trace info for interrupted .NET thread.
	void GetNetStackTrace(CUTF8EncStream& rEncStream);
	/// Get stack trace info for interrupted .NET thread.
	void GetNetStackTrace(CXmlWriter& rXmlWriter);
#endif
	/// Get stack trace info for all threads.
	void GetWin32ThreadsList(CUTF8EncStream& rEncStream, CEnumProcess* pEnumProcess);
	/// Get stack trace info for all threads.
	void GetWin32ThreadsList(CXmlWriter& rXmlWriter, CEnumProcess* pEnumProcess);
#ifdef _MANAGED
	/// Get stack trace info for all threads.
	void GetNetThreadsList(CUTF8EncStream& rEncStream);
	/// Get stack trace info for all threads.
	void GetNetThreadsList(CXmlWriter& rXmlWriter);
#endif
	/// Get module list for specified process.
	void GetModuleList(CUTF8EncStream& rEncStream, CEnumProcess* pEnumProcess, CEnumProcess::CProcessEntry& rProcEntry);
	/// Get module list for specified process.
	void GetModuleList(CXmlWriter& rXmlWriter, CEnumProcess* pEnumProcess, CEnumProcess::CProcessEntry& rProcEntry);
#ifdef _MANAGED
	/// Get assemblies list for current process.
	void GetAssemblyList(CUTF8EncStream& rEncStream);
	/// Get assemblies list for current process.
	void GetAssemblyList(CXmlWriter& rXmlWriter);
#endif
	/// Get process info for specified process.
	void GetProcessList(CUTF8EncStream& rEncStream, CEnumProcess* pEnumProcess);
	/// Get process info for specified process.
	void GetProcessList(CXmlWriter& rXmlWriter, CEnumProcess* pEnumProcess);
	/// Get OS information.
	static void GetOsInfo(COsInfo& rOsInfo);
	/// Get description of system CPUs.
	static void GetCpusInfo(CCpusInfo& rCpusInfo);
	/// Get description of specific CPU.
	static BOOL GetCpuInfo(DWORD dwCpuNum, CCpuInfo& rCpuInfo);
	/// Get system memory information.
	static void GetMemInfo(CMemInfo& rMemInfo);
	/// Get system error information.
	void GetSysErrorInfo(CSysErrorInfo& rErrorInfo);
	/// Get COM error reason.
	BOOL GetComErrorInfo(CComErrorInfo& rErrorInfo);
	/// Get crash location and reason.
	BOOL GetErrorInfo(CErrorInfo& rErrorInfo);
	/// Get string with CPU registers for the crash.
	void GetRegistersValues(CRegistersValues& rRegVals);
	/// Get text representation of topmost call stack entry.
	BOOL GetFirstWin32StackTraceString(CUTF8EncStream& rEncStream, HANDLE hThread = NULL);
	/// Get text representation of lower call stack entry.
	BOOL GetNextWin32StackTraceString(CUTF8EncStream& rEncStream);
#ifdef _MANAGED
	/// Get text representation of topmost call stack entry.
	BOOL GetFirstNetStackTraceString(CUTF8EncStream& rEncStream);
	/// Get text representation of lower call stack entry.
	BOOL GetNextNetStackTraceString(CUTF8EncStream& rEncStream);
#endif
	/// Get string with CPU registers for the crash.
	void GetRegistersString(CUTF8EncStream& rEncStream);
	/// Get crash location and reason.
	void GetWin32ErrorString(CUTF8EncStream& rEncStream);
#ifdef _MANAGED
	/// Get crash location and reason.
	void GetNetErrorString(CUTF8EncStream& rEncStream);
#endif
	/// Get crash location and reason.
	void GetErrorString(CUTF8EncStream& rEncStream);
	/// Get system error description.
	void GetSysErrorString(CUTF8EncStream& rEncStream);
	/// Get COM error reason.
	void GetComErrorString(CUTF8EncStream& rEncStream);
	/// Get description of system CPUs.
	static void GetCpuString(CUTF8EncStream& rEncStream);
	/// Get OS information.
	static void GetOsString(CUTF8EncStream& rEncStream);
	/// Get description of system memory.
	static void GetMemString(CUTF8EncStream& rEncStream);
	/// Get process environment strings.
	static void GetEnvironmentStrings(CUTF8EncStream& rEncStream);
	/// Get process environment strings.
	static void GetEnvironmentStrings(CXmlWriter& rXmlWriter);
	/// Get date-time string.
	void GetDateTime(PTSTR pszDateTime, DWORD dwDateTimeSize);
	/// Get time-stamp string.
	void GetTimeStamp(PTSTR pszTimeStamp, DWORD dwTimeStampSize);
	/// Generate comprehensive text information about the crash.
	void GetErrorLog(CUTF8EncStream& rEncStream, CEnumProcess* pEnumProcess);
	/// Generate comprehensive text information about the crash.
	void GetErrorLog(CXmlWriter& rXmlWriter, CEnumProcess* pEnumProcess);

	/// Protects the class from being accidentally copied.
	CSymEngine(const CSymEngine& rSymEngine);
	/// Protects the class from being accidentally copied.
	CSymEngine& operator=(const CSymEngine& rSymEngine);

public:
	/// Initialize the object.
	explicit CSymEngine(const CEngineParams& rParams);
	/// Destroy the object.
	~CSymEngine(void);
	/// Set engine parameters
	void SetEngineParameters(const CEngineParams& rParams);
	/// Reset engine parameters.
	void ResetEngineParameters(void);
	/// Returns true if DbgHelp.dll on user computer supports MiniDumpWriteDump() function.
	BOOL IsMiniDumpSupported(void) const;
	/// Writes report to file in compressed or uncompressed format.
	BOOL WriteReport(PCTSTR pszFileName, CEnumProcess* pEnumProcess);
	/// Archive report files.
	BOOL ArchiveReportFiles(PCTSTR pszReportFolder, PCTSTR pszArchiveFileName);
	/// Writes crash log and mini-dump to zip archive.
	BOOL WriteReportArchive(PCTSTR pszArchiveFileName, CEnumProcess* pEnumProcess);
	/// Write report files to a folder.
	BOOL WriteReportFiles(PCTSTR pszFolderName, CEnumProcess* pEnumProcess);
	/// Writes crash log to file.
	BOOL WriteLog(PCTSTR pszFileName, CEnumProcess* pEnumProcess);
	/// Writes crash dump to file.
	BOOL WriteDump(PCTSTR pszFileName);
	/// Writes screen-shot to file.
	BOOL WriteScreenShot(PCTSTR pszFileName);
	/// Get extension of report file.
	static PCTSTR GetReportFileExtension(void);
	/// Get extension of log file.
	static PCTSTR GetLogFileExtension(void);
	/// Automatically generate log/mini-dump file name according to the date and time of the crash.
	void GetReportFileName(PCTSTR pszExtension, PTSTR pszFileName, DWORD dwBufferSize) const;
	/// Automatically generate log/mini-dump file name according to the date and time of the crash.
	void GetReportFileName(PTSTR pszFileName, DWORD dwBufferSize) const;
	/// Get structured representation of topmost call stack entry.
	BOOL GetFirstStackTraceEntry(CStackTraceEntry& rEntry, HANDLE hThread = NULL);
	/// Get structured representation of lower call stack entry.
	BOOL GetNextStackTraceEntry(CStackTraceEntry& rEntry);
#ifdef _MANAGED
	/// Get structured representation of topmost call stack entry.
	BOOL GetFirstStackTraceEntry(CNetStackTrace::CNetStackTraceEntry& rEntry);
	/// Get structured representation of lower call stack entry.
	BOOL GetNextStackTraceEntry(CNetStackTrace::CNetStackTraceEntry& rEntry);
#endif
	/// Get string with CPU registers for the crash.
	void GetRegistersString(PTSTR pszRegString, DWORD dwRegStringSize);
	/// Get crash location and error reason.
	void GetWin32ErrorString(CStrStream& rStream);
#ifdef _MANAGED
	/// Get crash location and error reason.
	void GetNetErrorString(CStrStream& rStream);
#endif
	/// Get crash location and error reason.
	void GetErrorString(CStrStream& rStream);
	/// Get description of system CPUs.
	static void GetCpuString(CStrStream& rStream);
	/// Get user-friendly module version string.
	static BOOL GetVersionString(PCTSTR pszModuleName, PTSTR pszVersionString, DWORD dwVersionStringSize);
	/// Get OS information.
	static void GetOsString(PTSTR pszOSString, DWORD dwOSStringSize);
	/// Get description of system memory.
	static void GetMemString(PTSTR pszMemString, DWORD dwMemStringSize);
	/// Get process environment strings.
	static void GetEnvironmentStrings(CStrStream& rStream);
};

inline CSymEngine::CStackWalkContext::CStackWalkContext(void)
{
	ZeroMemory(this, sizeof(*this));
}

/**
 * @param hProcess - handle to the process.
 * @param pBaseAddress - base of memory area.
 * @param pBuffer - data buffer.
 * @param nSize - number of bytes to read.
 * @param pNumberOfBytesRead - number of bytes read.
 * @return if the function succeeds, the return value is nonzero.
 */
inline BOOL CALLBACK CSymEngine::ReadProcessMemoryProc64(HANDLE hProcess, DWORD64 pBaseAddress, PVOID pBuffer, DWORD nSize, PDWORD pNumberOfBytesRead)
{
	hProcess;
	return ReadProcessMemory(GetCurrentProcess(), (PVOID)pBaseAddress, pBuffer, nSize, pNumberOfBytesRead);
}

/**
 * @return true if DbgHelp.dll on client computer has MiniDumpWriteDump() function.
 */
inline BOOL CSymEngine::IsMiniDumpSupported(void) const
{
	return (FMiniDumpWriteDump != NULL);
}

inline CSymEngine::CScreenShot::~CScreenShot(void)
{
	delete[] (PBYTE)m_pBitsArray;
	delete[] (PBYTE)m_pBmpInfo;
}

inline CSymEngine::CSysErrorInfo::CSysErrorInfo(void)
{
	*m_szErrorCode = _T('\0');
	m_pszMessageBuffer = NULL;
}

inline CSymEngine::CSysErrorInfo::~CSysErrorInfo(void)
{
	FreeData();
}

inline void CSymEngine::CSysErrorInfo::FreeData(void)
{
	if (m_pszMessageBuffer)
		LocalFree(m_pszMessageBuffer);
}

/**
 * @param rErrorInfo - another error info object.
 */
inline CSymEngine::CSysErrorInfo::CSysErrorInfo(const CSysErrorInfo& rErrorInfo)
{
	CopyData(rErrorInfo);
}

/**
 * @param pszFileName - buffer for resulting file name.
 * @param dwBufferSize - size of file name buffer.
 */
inline void CSymEngine::GetReportFileName(PTSTR pszFileName, DWORD dwBufferSize) const
{
	PCTSTR pszReportExtension = GetReportFileExtension();
	GetReportFileName(pszReportExtension, pszFileName, dwBufferSize);
}

#ifndef _MANAGED

/**
 * @param rStream - stream object.
 */
inline void CSymEngine::GetErrorString(CStrStream& rStream)
{
	GetWin32ErrorString(rStream);
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
inline void CSymEngine::GetErrorString(CUTF8EncStream& rEncStream)
{
	GetWin32ErrorString(rEncStream);
}

#endif

#ifdef _MANAGED

/**
 * @param rEntry - stack entry information.
 * @return true if there is information about stack entry.
 */
inline BOOL CSymEngine::GetFirstStackTraceEntry(CNetStackTrace::CNetStackTraceEntry& rEntry)
{
	_ASSERTE(m_pNetStackTrace != NULL);
	return (m_pNetStackTrace != NULL ? m_pNetStackTrace->GetFirstStackTraceEntry(rEntry) : FALSE);
}

/**
 * @param rEntry - stack entry information.
 * @return true if there is information about stack entry.
 */
inline BOOL CSymEngine::GetNextStackTraceEntry(CNetStackTrace::CNetStackTraceEntry& rEntry)
{
	_ASSERTE(m_pNetStackTrace != NULL);
	return (m_pNetStackTrace != NULL ? m_pNetStackTrace->GetNextStackTraceEntry(rEntry) : FALSE);
}

/**
 * @param rEncStream - UTF-8 encoder object.
 * @return true if thread context was successfully resolved.
 */
inline BOOL CSymEngine::GetFirstNetStackTraceString(CUTF8EncStream& rEncStream)
{
	_ASSERTE(m_pNetStackTrace != NULL);
	return (m_pNetStackTrace != NULL ? m_pNetStackTrace->GetFirstStackTraceString(rEncStream) : FALSE);
}

/**
 * @param rEncStream - UTF-8 encoder object.
 * @return true if thread context was successfully resolved.
 */
inline BOOL CSymEngine::GetNextNetStackTraceString(CUTF8EncStream& rEncStream)
{
	_ASSERTE(m_pNetStackTrace != NULL);
	return (m_pNetStackTrace != NULL ? m_pNetStackTrace->GetNextStackTraceString(rEncStream) : FALSE);
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
inline void CSymEngine::GetNetThreadsList(CUTF8EncStream& rEncStream)
{
	rEncStream; // TBD
}

/**
 * @param rXmlWriter - XML writer.
 */
inline void CSymEngine::GetNetThreadsList(CXmlWriter& rXmlWriter)
{
	rXmlWriter; // TBD
}

#endif
