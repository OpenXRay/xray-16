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

#include "StdAfx.h"
#include "EnumProcess.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/// Template for the error message.
static const TCHAR szNAError[] = _T("N/A (%lu error)");

/// Custom parameter for Enum16Proc() function.
struct CEnum16Param
{
	/// Pointer to enumerator object.
	CEnumProcess* _this;
	/// NTVDM process ID.
	DWORD dwProcessID;
};


CEnumProcess::CModuleEntry::CModuleEntry(void)
{
	*m_szModuleName = _T('\0');
	m_pLoadBase = NULL;
	m_dwModuleSize = 0;
#ifdef READ_IMAGEHEADER
	m_pPreferredBase = NULL;
	m_dwPreferredSize = 0;
#endif
}

CEnumProcess::CProcessEntry::CProcessEntry(void)
{
	m_dwProcessID = 0;
	m_hTask16 = 0;
	*m_szProcessName = _T('\0');
}

CEnumProcess::CEnumProcess(void) :
				m_arrProcesses(0),
#ifdef USE_VDMDBG
				m_arrProcesses16(0),
#endif
				m_arrModules(0)
{
	m_hPsApiDll = LoadLibrary(_T("PSAPI.DLL"));
	if (m_hPsApiDll)
	{
		// Setup variables
		m_arrProcesses.EnsureSize(256, false);
		m_dwCurrentProcess = 0;
		m_arrModules.EnsureSize(256, false);
		m_dwCurrentModule = 0;

		// Find PSAPI functions
		FEnumProcesses = (PFEnumProcesses)GetProcAddress(m_hPsApiDll, "EnumProcesses");
		FEnumProcessModules = (PFEnumProcessModules)GetProcAddress(m_hPsApiDll, "EnumProcessModules");
#ifdef _UNICODE
		FGetModuleFileNameEx = (PFGetModuleFileNameEx)GetProcAddress(m_hPsApiDll, "GetModuleFileNameExW");
#else
		FGetModuleFileNameEx = (PFGetModuleFileNameEx)GetProcAddress(m_hPsApiDll, "GetModuleFileNameExA");
#endif

		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion >= 5) // Windows 2000 and newer
			if (osvi.dwMinorVersion == 0)
				m_dwSystemID = 2; // Windows 2000
			else
				m_dwSystemID = 4; // Windows XP and newer
		else
			m_dwSystemID = 8; // Windows NT
	}

	m_hKernelDll = GetModuleHandle(_T("KERNEL32.DLL"));
	if (m_hKernelDll)
	{
		// Setup variables
		m_hProcessSnap = INVALID_HANDLE_VALUE;
		m_hModuleSnap = INVALID_HANDLE_VALUE;
		m_hThreadSnap = INVALID_HANDLE_VALUE;

		// Find ToolHelp functions
		FCreateToolhelp32Snapshot = (PFCreateToolhelp32Snapshot)GetProcAddress(m_hKernelDll, "CreateToolhelp32Snapshot");
		FThread32First = (PFThread32First)GetProcAddress(m_hKernelDll, "Thread32First");
		FThread32Next = (PFThread32Next)GetProcAddress(m_hKernelDll, "Thread32Next");
#ifdef _UNICODE
		FProcess32First = (PFProcess32First)GetProcAddress(m_hKernelDll, "Process32FirstW");
		FProcess32Next = (PFProcess32Next)GetProcAddress(m_hKernelDll, "Process32NextW");
		FModule32First = (PFModule32First)GetProcAddress(m_hKernelDll, "Module32FirstW");
		FModule32Next = (PFModule32Next)GetProcAddress(m_hKernelDll, "Module32NextW");
#else
		FProcess32First = (PFProcess32First)GetProcAddress(m_hKernelDll, "Process32First");
		FProcess32Next = (PFProcess32Next)GetProcAddress(m_hKernelDll, "Process32Next");
		FModule32First = (PFModule32First)GetProcAddress(m_hKernelDll, "Module32First");
		FModule32Next = (PFModule32Next)GetProcAddress(m_hKernelDll, "Module32Next");
#endif
	}

#ifdef USE_VDMDBG
	m_hVmDbgDll = LoadLibrary(_T("VDMDBG.DLL"));
	if (m_hVmDbgDll)
	{
		// Find VDMdbg functions
		FVDMEnumTaskWOWEx = (PFVDMEnumTaskWOWEx)GetProcAddress(m_hVmDbgDll, "VDMEnumTaskWOWEx");
		m_arrProcesses16.EnsureSize(256, false);
		m_dwCurrentProcess16 = 0;
	}
#endif

	m_dwAvailMethods = EM_NONE;
	// Does all PSAPI functions exist?
	if (m_hPsApiDll &&
		FEnumProcesses &&
		FEnumProcessModules &&
		FGetModuleFileNameEx)
	{
		m_dwAvailMethods |= EM_PSAPI;
	}

	// How about ToolHelp?
	if (m_hKernelDll &&
		FCreateToolhelp32Snapshot &&
		FProcess32Next && FProcess32Next &&
		FModule32First && FModule32Next &&
		FThread32First && FThread32Next)
	{
		m_dwAvailMethods |= EM_TOOLHELP;
	}

#ifdef USE_VDMDBG
	// And 16-bit enumeration?
	if (m_hVmDbgDll &&
		FVDMEnumTaskWOWEx &&
		m_dwAvailMethods != EM_NONE)
	{
		m_dwAvailMethods |= EM_PROC16;
	}
#endif

	// Find the preferred method of enumeration
	m_dwMethod = EM_NONE;
	if (m_dwAvailMethods & EM_TOOLHELP)
		m_dwMethod = EM_TOOLHELP;
	else if (m_dwAvailMethods & EM_PSAPI)
		m_dwMethod = EM_PSAPI;
#ifdef USE_VDMDBG
	if (m_dwAvailMethods & EM_PROC16)
		m_dwMethod |= EM_PROC16;
#endif
}

CEnumProcess::~CEnumProcess(void)
{
	Close();

	if (m_hPsApiDll)
		FreeLibrary(m_hPsApiDll);

#ifdef USE_VDMDBG
	if (m_hVmDbgDll)
		FreeLibrary(m_hVmDbgDll);
#endif
}

void CEnumProcess::Close(void)
{
	if (m_hKernelDll)
	{
		if (m_hThreadSnap != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hThreadSnap);
			m_hThreadSnap = INVALID_HANDLE_VALUE;
		}
		if (m_hModuleSnap != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hModuleSnap);
			m_hModuleSnap = INVALID_HANDLE_VALUE;
		}
		if (m_hProcessSnap != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hProcessSnap);
			m_hProcessSnap = INVALID_HANDLE_VALUE;
		}
	}

	m_arrModules.DeleteAll(true);
#ifdef USE_VDMDBG
	m_arrProcesses16.DeleteAll(true);
#endif
	m_arrProcesses.DeleteAll(true);
}

/**
 * @param dwMethod - desirable enumeration method.
 */
void CEnumProcess::SetMethod(DWORD dwMethod)
{
	if ((m_dwAvailMethods & dwMethod) == dwMethod)
#ifdef USE_VDMDBG
		if (dwMethod != EM_PROC16)
#endif
    		m_dwMethod = dwMethod;
}

/**
 * @param rEntry - process description entry.
 * @return true if there is information about process.
 */
BOOL CEnumProcess::GetCurrentProcess(CProcessEntry& rEntry)
{
	m_arrProcesses.DeleteAll();
	m_dwCurrentProcess = 0;
	rEntry.m_hTask16 = NULL;
	rEntry.m_dwProcessID = GetCurrentProcessId();
	GetModuleFileName(NULL, rEntry.m_szProcessName, countof(rEntry.m_szProcessName));
	PCTSTR pszProcessName = PathFindFileName(rEntry.m_szProcessName);
	DWORD dwProcessName = _tcslen(pszProcessName);
	MoveMemory(rEntry.m_szProcessName, pszProcessName, (dwProcessName + 1) * sizeof(TCHAR));
	return TRUE;
}

/**
 * @param rEntry - process description entry.
 * @return true if there is information about process.
 */
BOOL CEnumProcess::GetProcessFirst(CEnumProcess::CProcessEntry& rEntry)
{
	if (m_dwMethod == EM_NONE)
		return FALSE;
	if (m_dwMethod & EM_TOOLHELP)
	{
		// Use ToolHelp functions
		if (m_hProcessSnap != INVALID_HANDLE_VALUE)
			CloseHandle(m_hProcessSnap);
		m_hProcessSnap = FCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (m_hProcessSnap == INVALID_HANDLE_VALUE)
			return FALSE;
		m_pe.dwSize = sizeof(m_pe);
		if (! FProcess32First(m_hProcessSnap, &m_pe))
			return FALSE;
		rEntry.m_dwProcessID = m_pe.th32ProcessID;
		_tcscpy_s(rEntry.m_szProcessName, countof(rEntry.m_szProcessName), m_pe.szExeFile);
		return TRUE;
	}
	else
	{
		// Use PSAPI functions
		BOOL bResult;
		DWORD cbNeeded;
		DWORD dwArraySize = m_arrProcesses.GetSize();
		m_arrProcesses.SetCount(dwArraySize);

		for (;;)
		{
			cbNeeded = 0;
			dwArraySize = m_arrProcesses.GetCount();
			DWORD dwBufferSize = dwArraySize * sizeof(DWORD);
			bResult = FEnumProcesses(m_arrProcesses, dwBufferSize, &cbNeeded);

			// We might need more memory here
			if (cbNeeded <= dwBufferSize)
				break;
			// Try again
			m_arrProcesses.SetCount(dwArraySize + dwArraySize);
			if (dwArraySize == (DWORD)m_arrProcesses.GetCount())
				break;
		}

		m_dwCurrentProcess = 0;
		if (! bResult)
			return FALSE;
		m_arrProcesses.SetCount(cbNeeded / sizeof(DWORD));
		return FillPStructPSAPI(m_arrProcesses[(int)(m_dwCurrentProcess++)], rEntry);
	}
}

/**
 * @param rEntry - process description entry.
 * @return true if there is information about process.
 */
BOOL CEnumProcess::GetProcessNext(CEnumProcess::CProcessEntry& rEntry)
{
	if (m_dwMethod == EM_NONE)
		return FALSE;

#ifdef USE_VDMDBG
	// We have some 16-bit processes to get
	if ((int)m_dwCurrentProcess16 < m_arrProcesses16.GetCount())
	{
		rEntry = m_arrProcesses16[(int)(m_dwCurrentProcess16++)];
		return TRUE;
	}
	rEntry.m_hTask16 = 0;
#endif

	if (m_dwMethod & EM_TOOLHELP)
	{
		// Use ToolHelp functions
		m_pe.dwSize = sizeof(m_pe);
		if (! FProcess32Next(m_hProcessSnap, &m_pe))
			return FALSE;
		rEntry.m_dwProcessID = m_pe.th32ProcessID;
		_tcscpy_s(rEntry.m_szProcessName, countof(rEntry.m_szProcessName), m_pe.szExeFile);
	}
	else
	{
		// Use PSAPI functions
		if ((int)m_dwCurrentProcess >= m_arrProcesses.GetCount())
			return FALSE;
		if (! FillPStructPSAPI(m_arrProcesses[(int)(m_dwCurrentProcess++)], rEntry))
			return FALSE;
	}

#ifdef USE_VDMDBG
	// Check for 16-bit processes?
	if (EM_PROC16 & m_dwMethod)
	{
		// Is this a NTVDM?
		static const TCHAR szNTVDM[] = _T("NTVDM.EXE");
		static const DWORD dwNTVDMLen = countof(szNTVDM) - 1;
		DWORD dwProcessName = _tcslen(rEntry.m_szProcessName);
		if (dwProcessName >= dwNTVDMLen &&
			! _tcsicmp(rEntry.m_szProcessName + (dwProcessName - dwNTVDMLen), szNTVDM))
		{
			m_arrProcesses16.DeleteAll();
			CEnum16Param param;
			param._this = this;
			param.dwProcessID = rEntry.m_dwProcessID;
			FVDMEnumTaskWOWEx(rEntry.m_dwProcessID, (TASKENUMPROCEX)Enum16Proc, (LPARAM)&param);
		}
	}
#endif

	return TRUE;
}

/**
 * @param dwProcessID - process ID.
 * @param rEntry - module description entry.
 * @return true if there is information about module.
 */
BOOL CEnumProcess::GetModuleFirst(DWORD dwProcessID, CEnumProcess::CModuleEntry& rEntry)
{
	if (m_dwMethod == EM_NONE)
		return FALSE;
	if (m_dwMethod & EM_TOOLHELP)
	{
		// Use ToolHelp functions
		if (m_hModuleSnap != INVALID_HANDLE_VALUE)
			CloseHandle(m_hModuleSnap);
		m_hModuleSnap = FCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessID);
		if (m_hModuleSnap == INVALID_HANDLE_VALUE)
			return FALSE;
		m_me.dwSize = sizeof(m_me);
		if(! FModule32First(m_hModuleSnap, &m_me))
			return FALSE;
		_tcscpy_s(rEntry.m_szModuleName, countof(rEntry.m_szModuleName), m_me.szExePath);
		rEntry.m_pLoadBase = m_me.modBaseAddr;
		rEntry.m_dwModuleSize = m_me.modBaseSize;
#ifdef READ_IMAGEHEADER
		GetPreferredModuleLocation(dwProcessID, m_me.modBaseAddr, rEntry.m_pPreferredBase, rEntry.m_dwPreferredSize);
#endif
		return TRUE;
	}
	else
	{
		// Use PSAPI functions
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
		if (hProcess != NULL)
		{
			BOOL bResult;
			DWORD cbNeeded;
			DWORD dwArraySize = m_arrModules.GetSize();
			m_arrModules.SetCount(dwArraySize);

			for (;;)
			{
				cbNeeded = 0;
				dwArraySize = m_arrModules.GetCount();
				DWORD dwBufferSize = dwArraySize * sizeof(HMODULE);
				bResult = FEnumProcessModules(hProcess, m_arrModules, dwBufferSize, &cbNeeded);

				// We might need more memory here
				if (cbNeeded <= dwBufferSize)
					break;
				// Try again
				m_arrModules.SetCount(dwArraySize + dwArraySize);
				if (dwArraySize == (DWORD)m_arrModules.GetCount())
					break;
			}

			CloseHandle(hProcess);
			m_dwCurrentModule = 0;
			if (! bResult)
				return FALSE;
			m_arrModules.SetCount(cbNeeded / sizeof(HMODULE));
			return FillMStructPSAPI(dwProcessID, m_arrModules[(int)(m_dwCurrentModule++)], rEntry);
		}
		return FALSE;
	}
}

/**
 * @param dwProcessID - process ID.
 * @param rEntry - module description entry.
 * @return true if there is information about module.
 */
BOOL CEnumProcess::GetModuleNext(DWORD dwProcessID, CEnumProcess::CModuleEntry& rEntry)
{
	if (m_dwMethod == EM_NONE)
		return FALSE;
	if (m_dwMethod & EM_TOOLHELP)
	{
		// Use ToolHelp functions
		m_me.dwSize = sizeof(m_me);
		if(! FModule32Next(m_hModuleSnap, &m_me))
			return FALSE;
		_tcscpy_s(rEntry.m_szModuleName, countof(rEntry.m_szModuleName), m_me.szExePath);
		rEntry.m_pLoadBase = m_me.modBaseAddr;
		rEntry.m_dwModuleSize = m_me.modBaseSize;
#ifdef READ_IMAGEHEADER
		GetPreferredModuleLocation(dwProcessID, m_me.modBaseAddr, rEntry.m_pPreferredBase, rEntry.m_dwPreferredSize);
#endif
		return TRUE;
	}
	else
	{
		// Use PSAPI functions
		if ((int)m_dwCurrentModule >= m_arrModules.GetCount())
			return FALSE;
		return FillMStructPSAPI(dwProcessID, m_arrModules[(int)(m_dwCurrentModule++)], rEntry);
	}
}

/**
 * @param dwProcessID - process ID.
 * @param rEntry - thread description entry.
 * @return true if there is information about thread.
 */
BOOL CEnumProcess::GetThreadFirst(DWORD dwProcessID, CEnumProcess::CThreadEntry& rEntry)
{
	if (m_dwMethod == EM_NONE)
		return FALSE;
	if (m_dwMethod & EM_TOOLHELP)
	{
		// Use ToolHelp functions
		if (m_hThreadSnap != INVALID_HANDLE_VALUE)
			CloseHandle(m_hThreadSnap);
		m_hThreadSnap = FCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (m_hThreadSnap == INVALID_HANDLE_VALUE)
			return FALSE;
		m_te.dwSize = sizeof(m_te);
		if(! FThread32First(m_hThreadSnap, &m_te))
			return FALSE;
		while (m_te.th32OwnerProcessID != dwProcessID)
		{
			m_te.dwSize = sizeof(m_te);
			if(! FThread32Next(m_hThreadSnap, &m_te))
				return FALSE;
		}
		rEntry.m_dwThreadID = m_te.th32ThreadID;
		return TRUE;
	}
	else
	{
		// PSAPI functions don't support threads enumeration.
		return FALSE;
	}
}

/**
 * @param dwProcessID - process ID.
 * @param rEntry - thread description entry.
 * @return true if there is information about thread.
 */
BOOL CEnumProcess::GetThreadNext(DWORD dwProcessID, CEnumProcess::CThreadEntry& rEntry)
{
	if (m_dwMethod == EM_NONE)
		return FALSE;
	if (m_dwMethod & EM_TOOLHELP)
	{
		// Use ToolHelp functions
		do
		{
			m_te.dwSize = sizeof(m_te);
			if(! FThread32Next(m_hThreadSnap, &m_te))
				return FALSE;
		}
		while (m_te.th32OwnerProcessID != dwProcessID);
		rEntry.m_dwThreadID = m_te.th32ThreadID;
		return TRUE;
	}
	else
	{
		// PSAPI functions don't support threads enumeration.
		return FALSE;
	}
}

/**
 * @param dwProcessID - process ID.
 * @param rEntry - process description entry.
 * @return true if there is information about process.
 */
BOOL CEnumProcess::FillPStructPSAPI(DWORD dwProcessID, CEnumProcess::CProcessEntry& rEntry)
{
	rEntry.m_dwProcessID = dwProcessID;
	// Open process to get file name
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
	if (hProcess != NULL)
	{
		HMODULE hModule;
		DWORD dwSize = 0;
		// Get the first module (the process itself)
		if ((FEnumProcessModules(hProcess, &hModule, sizeof(hModule), &dwSize) || dwSize > 0) &&
			FGetModuleFileNameEx(hProcess, hModule, rEntry.m_szProcessName, countof(rEntry.m_szProcessName)))
		{
			PCTSTR pszProcessName = PathFindFileName(rEntry.m_szProcessName);
			DWORD dwProcessName = _tcslen(pszProcessName);
			MoveMemory(rEntry.m_szProcessName, pszProcessName, (dwProcessName + 1) * sizeof(TCHAR));
			CloseHandle(hProcess);
			return TRUE;
		}
		CloseHandle(hProcess);
	}
	if (dwProcessID == 0)
		_tcscpy_s(rEntry.m_szProcessName, countof(rEntry.m_szProcessName), _T("System Idle Process"));
	else if (dwProcessID == m_dwSystemID)
		_tcscpy_s(rEntry.m_szProcessName, countof(rEntry.m_szProcessName), _T("System"));
	else
		_stprintf_s(rEntry.m_szProcessName, countof(rEntry.m_szProcessName), szNAError, GetLastError());
	return TRUE;
}

/**
 * @param dwProcessID - process ID.
 * @param hModule - module handle.
 * @param rEntry - process description module.
 * @return true if there is information about module.
 */
BOOL CEnumProcess::FillMStructPSAPI(DWORD dwProcessID, HMODULE hModule, CEnumProcess::CModuleEntry& rEntry)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
	if (hProcess != NULL)
	{
		if (! FGetModuleFileNameEx(hProcess, hModule, rEntry.m_szModuleName, countof(rEntry.m_szModuleName)))
			_stprintf_s(rEntry.m_szModuleName, countof(rEntry.m_szModuleName), szNAError, GetLastError());
		rEntry.m_pLoadBase = (PVOID)hModule;
		rEntry.m_dwModuleSize = 0;
#ifdef READ_IMAGEHEADER
		GetPreferredModuleLocation(dwProcessID, (PVOID)hModule, rEntry.m_pPreferredBase, rEntry.m_dwPreferredSize);
#endif
		CloseHandle(hProcess);
		return TRUE;
	}
	return FALSE;
}

#ifdef READ_IMAGEHEADER

/**
 * @param dwProcessID - process ID.
 * @param pModBase - base module address.
 * @param pPreferredBase - pointer to preferred module load address.
 * @param dwPreferredSize - preferred module size.
 * @return true if operation has been successful.
 */
BOOL CEnumProcess::GetPreferredModuleLocation(DWORD dwProcessID, PVOID pModBase, PVOID& pPreferredBase, DWORD& dwPreferredSize)
{
	pPreferredBase = NULL;
	dwPreferredSize = 0;
	if (m_dwMethod == EM_NONE)
		return FALSE;
	HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, dwProcessID);
	if (hProcess == NULL)
		return FALSE;
	IMAGE_DOS_HEADER idh;
	BOOL bResult = FALSE;
	// Read DOS header
	ReadProcessMemory(hProcess, pModBase, &idh, sizeof(idh), NULL);
	if (idh.e_magic == IMAGE_DOS_SIGNATURE) // DOS header OK?
	{
		IMAGE_NT_HEADERS inh;
		// Read NT headers at offset e_lfanew
		ReadProcessMemory(hProcess, (PBYTE)pModBase + idh.e_lfanew, &inh, sizeof(inh), NULL);
		// NT signature OK?
		if (inh.Signature == IMAGE_NT_SIGNATURE)
		{
			bResult = TRUE;
			// Get the preferred base & size
			pPreferredBase = (PVOID)inh.OptionalHeader.ImageBase;
			dwPreferredSize = inh.OptionalHeader.SizeOfImage;
		}
	}
	CloseHandle(hProcess);
	return bResult;
}

#endif

#ifdef USE_VDMDBG

/**
 * @param dwThreadId - thread ID.
 * @param hModule16 - module handle.
 * @param hTask16 - 16-bit task handle.
 * @param pszModuleName - module name.
 * @param pszProcessName - process name.
 * @param lpUserDefined - user-defined parameter.
 * @return false to keep enumerating.
 */
BOOL CALLBACK CEnumProcess::Enum16Proc(DWORD dwThreadId, WORD hModule16, WORD hTask16, PSZ pszModuleName, PSZ pszProcessName, LPARAM lpUserDefined)
{
	dwThreadId; pszModuleName; hModule16;
	CEnum16Param* pParam = (CEnum16Param*)lpUserDefined;
	CEnumProcess* _this = pParam->_this;

	CEnumProcess::CProcessEntry& rEntry = _this->m_arrProcesses16.AddItem();
#ifdef _UNICODE
	MultiByteToWideChar(CP_ACP, 0, pszProcessName, -1, rEntry.m_szProcessName, countof(rEntry.m_szProcessName));
#else
	_tcscpy_s(rEntry.m_szProcessName, countof(rEntry.m_szProcessName), pszProcessName);
#endif
	rEntry.m_dwProcessID = pParam->dwProcessID;
	rEntry.m_hTask16 = hTask16;
	return FALSE;
}

#endif
