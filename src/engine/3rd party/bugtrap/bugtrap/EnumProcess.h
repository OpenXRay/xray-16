/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Process enumerator.
 * Updated by: Maksim Pyatkovskiy.
 * Note: Based on code developed by Moliate.
 * Downloaded from: http://www.codeproject.com/threads/enumprocess.asp
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "Array.h"

/// Comment out this if you don't want to use VDMDBG at all
//#define USE_VDMDBG
/// Comment out this if you don't want to read image header
//#define READ_IMAGEHEADER

#ifdef USE_VDMDBG
/// 16-bit enumeration functions loaded from VDMDBG.DLL.
typedef INT (WINAPI *PFVDMEnumTaskWOWEx)(DWORD dwProcessId, TASKENUMPROCEX fp, LPARAM lParam);
#endif

// Functions loaded from PSAPI.DLL.

/// Type definition of pointer to EnumProcesses() function.
typedef BOOL (WINAPI *PFEnumProcesses)(PDWORD pidProcess, DWORD cb, PDWORD pcbNeeded);
/// Type definition of pointer to EnumProcessModules() function.
typedef BOOL (WINAPI *PFEnumProcessModules)(HANDLE hProcess, HMODULE* phModule, DWORD cb, PDWORD pcbNeeded);
/// Type definition of pointer to GetModuleFileNameEx() function.
typedef DWORD (WINAPI *PFGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, PTSTR pszFileName, DWORD nSize);

// Functions loaded from KERNEL32.DLL.

/// Type definition of pointer to CreateToolhelp32Snapshot() function.
typedef HANDLE (WINAPI *PFCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
/// Type definition of pointer to Process32First() function.
typedef BOOL (WINAPI *PFProcess32First)(HANDLE hSnapshot, PPROCESSENTRY32 ppe);
/// Type definition of pointer to Process32Next() function.
typedef BOOL (WINAPI *PFProcess32Next)(HANDLE hSnapshot, PPROCESSENTRY32 ppe);
/// Type definition of pointer to Module32First() function.
typedef BOOL (WINAPI *PFModule32First)(HANDLE hSnapshot, PMODULEENTRY32 pme);
/// Type definition of pointer to Module32Next() function.
typedef BOOL (WINAPI *PFModule32Next)(HANDLE hSnapshot, PMODULEENTRY32 pme);
/// Type definition of pointer to Thread32First() function.
typedef BOOL (WINAPI *PFThread32First)(HANDLE hSnapshot, PTHREADENTRY32 pte);
/// Type definition of pointer to Thread32Next() function.
typedef BOOL (WINAPI *PFThread32Next)(HANDLE hSnapshot, PTHREADENTRY32 pte);

/// Enumerates processes in a system.
class CEnumProcess
{
public:
	/// Possible enumeration methods.
	enum ENUM_METHOD
	{
		/// No methods are defined.
		EM_NONE     = 0x00,
#ifdef USE_VDMDBG
		/// PROC16 method (Windows NT and Windows 2000).
		EM_PROC16   = 0x01,
#endif
		/// PSAPI method (Windows NT and Windows 2000).
		EM_PSAPI    = 0x02,
		/// Tool Help (Windows 9x and Windows 2000).
		EM_TOOLHELP = 0x04
	};

	/// Stores process information.
	struct CProcessEntry
	{
		/// Process name (file name of main module).
		TCHAR m_szProcessName[MAX_PATH];
		/// Process ID.
		DWORD m_dwProcessID;
		/// Task handle for 16-bit process.
		WORD m_hTask16;
		/// Initialize the object.
		CProcessEntry(void);
	};

	/// Stores thread information.
	struct CThreadEntry
	{
		/// Thread ID.
		DWORD m_dwThreadID;
		/// Initialize the object.
		CThreadEntry(void);
	};

	/// Stores module information.
	struct CModuleEntry
	{
		/// Module file name.
		TCHAR m_szModuleName[MAX_PATH];
		/// Module base address.
		PVOID m_pLoadBase;
		/// Module size.
		DWORD m_dwModuleSize;
#ifdef READ_IMAGEHEADER
		/// Preferred base address.
		PVOID m_pPreferredBase;
		/// Preferred module size.
		DWORD m_dwPreferredSize;
#endif
		/// Initialize the object.
		CModuleEntry(void);
	};

	/// Initialize the object.
	CEnumProcess(void);
	/// Destroy the object.
	~CEnumProcess(void);
	/// Free used memory.
	void Close(void);

	/// Get first module for the process. Should obviously be called before GetModuleNext().
	BOOL GetModuleFirst(DWORD dwProcessID, CModuleEntry& rEntry);
	/// Get next module for the process.
	BOOL GetModuleNext(DWORD dwProcessID, CModuleEntry& rEntry);
	/// Get first thread for the process.
	BOOL GetThreadFirst(DWORD dwProcessID, CThreadEntry& rEntry);
	/// Get next thread for the process.
	BOOL GetThreadNext(DWORD dwProcessID, CThreadEntry& rEntry);
	/// Retrieves current process information.
	BOOL GetCurrentProcess(CProcessEntry& rEntry);
	/// Retrieves the first process in the enumeration. Should obviously be called before GetProcessNext().
	BOOL GetProcessFirst(CProcessEntry& rEntry);
	/// Returns the following process.
	BOOL GetProcessNext(CProcessEntry& rEntry);

	/// Get available enumeration methods.
	DWORD GetAvailableMethods(void) const;
	/// Get suggested enumeration method.
	DWORD GetSuggestedMethod(void) const;
	/// Set enumeration method.
	void SetMethod(DWORD dwMethod);

private:
	/// Protects the class from being accidentally copied.
	CEnumProcess(const CEnumProcess& rEnumProcess);
	/// Protects the class from being accidentally copied.
	CEnumProcess& operator=(const CEnumProcess& rEnumProcess);

#ifdef READ_IMAGEHEADER
	/// Get preferred base address of the module.
	BOOL GetPreferredModuleLocation(DWORD dwProcessID, PVOID pModBase, PVOID& pPreferredBase, DWORD& dwPreferredSize);
#endif

	/// Enumeration method.
	DWORD m_dwMethod;
	/// Bit-mask of available enumeration methods.
	DWORD m_dwAvailMethods;

	/// Handle to the module (PSAPI.DLL).
	HMODULE m_hPsApiDll;
	/// Array of process IDs.
	CArray<DWORD> m_arrProcesses;
	/// Pointer to current process identifier.
	DWORD m_dwCurrentProcess;
	/// Array of module handles.
	CArray<HMODULE> m_arrModules;
	/// Pointer to current module handle.
	DWORD m_dwCurrentModule;
	/// PID of system process.
	DWORD m_dwSystemID;

	/// Pointer to EnumProcess() function.
	PFEnumProcesses FEnumProcesses;
	/// Pointer to EnumProcessModules() function.
	PFEnumProcessModules FEnumProcessModules;
	/// Pointer to GetModuleFileNameEx() function.
	PFGetModuleFileNameEx FGetModuleFileNameEx;
	/// Fill in information about the process.
	BOOL FillPStructPSAPI(DWORD dwProcessID, CProcessEntry& rEntry);
	/// Fill in information about the module.
	BOOL FillMStructPSAPI(DWORD dwProcessID, HMODULE hModule, CModuleEntry& rEntry);

	/// Handle to the module (KERNEL32.DLL).
	HMODULE m_hKernelDll;
	/// Process snapshot handles.
	HANDLE m_hProcessSnap;
	/// Module snapshot handles.
	HANDLE m_hModuleSnap;
	/// Thread snapshot handles.
	HANDLE m_hThreadSnap;
	/// Process information.
	PROCESSENTRY32 m_pe;
	/// Module information.
	MODULEENTRY32 m_me;
	/// Thread information.
	THREADENTRY32 m_te;

	/// Pointer to CreateToolhelp32Snapshot() function.
	PFCreateToolhelp32Snapshot FCreateToolhelp32Snapshot;
	/// Pointer to Process32First() function.
	PFProcess32First FProcess32First;
	/// Pointer to Process32Next() function.
	PFProcess32Next FProcess32Next;
	/// Pointer to Module32First() function.
	PFModule32First FModule32First;
	/// Pointer to Module32Next() function.
	PFModule32Next FModule32Next;
	/// Pointer to Thread32First() function.
	PFThread32First FThread32First;
	/// Pointer to Thread32Next() function.
	PFThread32Next FThread32Next;

#ifdef USE_VDMDBG
	/// List of 16-bit processes.
	CArray<CProcessEntry> m_arrProcesses16;
	/// Pointer to current 16-bit process.
	DWORD m_dwCurrentProcess16;
	/// Handle to module VDMDBG.DLL.
	HMODULE m_hVmDbgDll;
	/// 16-bit enumeration.
	PFVDMEnumTaskWOWEx FVDMEnumTaskWOWEx;

	/// Enumerator for 16-bit processes.
	static BOOL CALLBACK Enum16Proc(DWORD dwThreadId, WORD hModule16, WORD hTask16, PSZ pszModuleName, PSZ pszProcessName, LPARAM lpUserDefined);
#endif
};

/**
 * @return preferred enumeration method for current system.
 */
inline DWORD CEnumProcess::GetSuggestedMethod(void) const
{
	return m_dwMethod;
}

/**
 * @return bit-mask of available enumeration methods.
 */
inline DWORD CEnumProcess::GetAvailableMethods(void) const
{
	return m_dwAvailMethods;
}

inline CEnumProcess::CThreadEntry::CThreadEntry(void)
{
	m_dwThreadID = 0;
}
