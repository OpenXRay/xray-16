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

#include "StdAfx.h"
#include "SymEngine.h"
#include "BugTrap.h"
#include "BugTrapUtils.h"
#include "TextFormat.h"
#include "Globals.h"
#include "MemStream.h"
#include "FileStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAX_FRAME_COUNT   1000

CSymEngine::CEngineParams::CEngineParams(void)
{
	GetLocalTime(&m_DateTime);
	m_pExceptionPointers = NULL;
	m_eExceptionType = NO_EXCEPTION;
	m_dwLastError = NOERROR;
	MakeScreenShot();
}

/**
 * @param pExceptionPointers - current exception information.
 * @param eExceptionType - exception type.
 */
CSymEngine::CEngineParams::CEngineParams(PEXCEPTION_POINTERS pExceptionPointers, EXCEPTION_TYPE eExceptionType)
{
	_ASSERTE(pExceptionPointers != NULL && eExceptionType != NO_EXCEPTION);
	m_dwLastError = GetLastError();
	GetLocalTime(&m_DateTime);
	m_pExceptionPointers = pExceptionPointers;
	m_eExceptionType = eExceptionType;
	::GetErrorInfo(0, &m_pErrorInfo);
	MakeScreenShot();
}

void CSymEngine::CEngineParams::MakeScreenShot(void)
{
	if ((g_dwFlags & (BTF_DETAILEDMODE | BTF_SCREENCAPTURE)) == (BTF_DETAILEDMODE | BTF_SCREENCAPTURE))
		m_pScreenShot = new CScreenShot();
	else
		m_pScreenShot = NULL;
}

CSymEngine::CStackTraceEntry::CStackTraceEntry(void)
{
	*m_szModule = _T('\0');
	*m_szAddress = _T('\0');
	*m_szFunctionName = _T('\0');
	*m_szFunctionOffset = _T('\0');
	*m_szFunctionInfo = _T('\0');
	*m_szSourceFile = _T('\0');
	*m_szLineNumber = _T('\0');
	*m_szLineOffset = _T('\0');
	*m_szLineInfo = _T('\0');
}

CSymEngine::CErrorInfo::CErrorInfo(void)
{
	m_pszWhat = NULL;
	*m_szProcessName = _T('\0');
	*m_szProcessID = _T('\0');
}

CSymEngine::CCpusInfo::CCpusInfo(void)
{
	m_pszCpuArch = NULL;
	m_dwNumCpus = 0;
}

CSymEngine::CCpuInfo::CCpuInfo(void)
{
	*m_szCpuId = _T('\0');
	*m_szCpuSpeed = _T('\0');
	*m_szCpuDescription = _T('\0');
}

CSymEngine::COsInfo::COsInfo(void)
{
	m_pszWinVersion = NULL;
	*m_szSPVersion = _T('\0');
	*m_szBuildNumber = _T('\0');
#ifdef _MANAGED
	*m_szNetVersion = _T('\0');
#endif
}

CSymEngine::CMemInfo::CMemInfo(void)
{
	*m_szMemoryLoad = _T('\0');
	*m_szTotalPhys = _T('\0');
	*m_szAvailPhys = _T('\0');
	*m_szTotalPageFile = _T('\0');
	*m_szAvailPageFile = _T('\0');
}

/**
 * @param rErrorInfo - another error info object.
 */
void CSymEngine::CSysErrorInfo::CopyData(const CSysErrorInfo& rErrorInfo)
{
	_tcscpy_s(m_szErrorCode, countof(m_szErrorCode), rErrorInfo.m_szErrorCode);
	if (rErrorInfo.m_pszMessageBuffer)
	{
		DWORD dwMessageSize = _tcslen(rErrorInfo.m_pszMessageBuffer) + 1;
		m_pszMessageBuffer = (PTSTR)LocalAlloc(LMEM_FIXED, dwMessageSize);
		if (m_pszMessageBuffer)
			_tcscpy_s(m_pszMessageBuffer, dwMessageSize, rErrorInfo.m_pszMessageBuffer);
	}
	else
		m_pszMessageBuffer = NULL;
}

/**
 * @param rErrorInfo - another error info object.
 * @return reference to this object.
 */
CSymEngine::CSysErrorInfo& CSymEngine::CSysErrorInfo::operator=(const CSysErrorInfo& rErrorInfo)
{
	FreeData();
	CopyData(rErrorInfo);
	return *this;
}

CSymEngine::CRegistersValues::CRegistersValues(void)
{
	*m_szEax = _T('\0');
	*m_szEbx = _T('\0');
	*m_szEcx = _T('\0');
	*m_szEdx = _T('\0');
	*m_szEsi = _T('\0');
	*m_szEdi = _T('\0');
	*m_szEsp = _T('\0');
	*m_szEbp = _T('\0');
	*m_szEip = _T('\0');
	*m_szSegCs = _T('\0');
	*m_szSegDs = _T('\0');
	*m_szSegSs = _T('\0');
	*m_szSegEs = _T('\0');
	*m_szSegFs = _T('\0');
	*m_szSegGs = _T('\0');
	*m_szEFlags = _T('\0');
}

/**
 * @param rParams - symbolic engine parameters.
 */
void CSymEngine::SetEngineParameters(const CEngineParams& rParams)
{
	m_pExceptionPointers = rParams.m_pExceptionPointers;
	m_dwLastError = rParams.m_dwLastError;
	m_DateTime = rParams.m_DateTime;
	m_eExceptionType = rParams.m_eExceptionType;
	m_dwExceptionAddress = m_pExceptionPointers != NULL ? (DWORD64)m_pExceptionPointers->ExceptionRecord->ExceptionAddress : MAXULONG_PTR;
	m_pScreenShot = rParams.m_pScreenShot;
	m_pErrorInfo = rParams.m_pErrorInfo;
	AdjustExceptionStackFrame();
#ifdef _MANAGED
	if (m_pExceptionPointers != NULL)
		m_pNetStackTrace = NetThunks::IsNetException() ? new CNetStackTrace() : NULL;
	else
		m_pNetStackTrace = new CNetStackTrace(NetThunks::GetCurrentThread());
#endif
}

void CSymEngine::ResetEngineParameters(void)
{
	m_pExceptionPointers = NULL;
	m_dwLastError = NOERROR;
	m_eExceptionType = NO_EXCEPTION;
	m_dwExceptionAddress = MAXULONG_PTR;
	m_pScreenShot = NULL;
	m_pErrorInfo = NULL;
	GetLocalTime(&m_DateTime);
#ifdef _MANAGED
	delete m_pNetStackTrace;
	m_pNetStackTrace = NULL;
#endif
}

/**
 * @param rParams - symbolic engine parameters.
 */
CSymEngine::CSymEngine(const CEngineParams& rParams)
{
	static const TCHAR szKernelDll[] = _T("KERNEL32.DLL");
	m_hKernelDll = GetModuleHandle(szKernelDll);
	FOpenThread = m_hKernelDll ? (PFOpenThread)GetProcAddress(m_hKernelDll, "OpenThread") : NULL;

	static const TCHAR szDbgHelDll[] = _T("DBGHELP.DLL");
	TCHAR szDbgHelpPath[MAX_PATH];
	GetModuleFileName(g_hInstance, szDbgHelpPath, countof(szDbgHelpPath));
	PathRemoveFileSpec(szDbgHelpPath);
	PathAppend(szDbgHelpPath, szDbgHelDll);

	m_hDbgHelpDll = LoadLibrary(szDbgHelpPath);
	if (m_hDbgHelpDll == NULL)
		m_hDbgHelpDll = LoadLibrary(szDbgHelDll);

	m_hSymProcess = NULL;
#ifdef _MANAGED
	m_pNetStackTrace = NULL;
#endif

	if (m_hDbgHelpDll != NULL)
	{
		FSymGetOptions = (PFSymGetOptions)GetProcAddress(m_hDbgHelpDll, "SymGetOptions");
		FSymSetOptions = (PFSymSetOptions)GetProcAddress(m_hDbgHelpDll, "SymSetOptions");
		FSymInitialize = (PFSymInitialize)GetProcAddress(m_hDbgHelpDll, "SymInitialize");
		FSymCleanup = (PFSymCleanup)GetProcAddress(m_hDbgHelpDll, "SymCleanup");
		FSymGetModuleBase64 = (PFSymGetModuleBase64)GetProcAddress(m_hDbgHelpDll, "SymGetModuleBase64");
		FSymFromAddr = (PFSymFromAddr)GetProcAddress(m_hDbgHelpDll, "SymFromAddr");
		FSymGetLineFromAddr64 = (PFSymGetLineFromAddr64)GetProcAddress(m_hDbgHelpDll, "SymGetLineFromAddr64");
		FStackWalk64 = (PFStackWalk64)GetProcAddress(m_hDbgHelpDll, "StackWalk64");
		FSymFunctionTableAccess64 = (PFSymFunctionTableAccess64)GetProcAddress(m_hDbgHelpDll, "SymFunctionTableAccess64");
		FMiniDumpWriteDump = (PFMiniDumpWriteDump)GetProcAddress(m_hDbgHelpDll, "MiniDumpWriteDump");

		if (FSymGetOptions && FSymSetOptions && FSymInitialize && FSymCleanup &&
		    FSymGetModuleBase64 && FSymFromAddr && FSymGetLineFromAddr64 &&
		    FStackWalk64 && FSymFunctionTableAccess64 && FMiniDumpWriteDump)
		{
			DWORD dwOptions = FSymGetOptions();
			FSymSetOptions(dwOptions | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

			m_hSymProcess = g_bWinNT ? GetCurrentProcess() : (HANDLE)GetCurrentProcessId();
			if (FSymInitialize(m_hSymProcess, NULL, TRUE))
				SetEngineParameters(rParams);
			else
				m_hSymProcess = NULL;
		}
	}
	else
	{
		FSymGetOptions = NULL;
		FSymSetOptions = NULL;
		FSymInitialize = NULL;
		FSymCleanup = NULL;
		FSymGetModuleBase64 = NULL;
		FSymFromAddr = NULL;
		FSymGetLineFromAddr64 = NULL;
		FStackWalk64 = NULL;
		FSymFunctionTableAccess64 = NULL;
		FMiniDumpWriteDump = NULL;
	}

	if (m_hSymProcess == NULL)
		ResetEngineParameters();
}

CSymEngine::~CSymEngine(void)
{
#ifdef _MANAGED
	delete m_pNetStackTrace;
#endif
	if (m_hSymProcess != NULL)
		FSymCleanup(m_hSymProcess);
	if (m_hDbgHelpDll != NULL)
		FreeLibrary(m_hDbgHelpDll);
}

CSymEngine::CScreenShot::CScreenShot(void)
{
	BOOL bResult = FALSE;
	ZeroMemory(this, sizeof(*this));
	HDC hDisplayDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	if (hDisplayDC)
	{
		int nWidth = GetDeviceCaps(hDisplayDC, HORZRES);
		int nHeight = GetDeviceCaps(hDisplayDC, VERTRES);
		HBITMAP hBitmap = CreateCompatibleBitmap(hDisplayDC, nWidth, nHeight);
		if (hBitmap)
		{
			HDC hMemDC = CreateCompatibleDC(hDisplayDC);
			if (hMemDC)
			{
				HBITMAP hbmpSafeBitmap = SelectBitmap(hMemDC, hBitmap);
				BitBlt(hMemDC, 0, 0, nWidth, nHeight, hDisplayDC, 0, 0, SRCCOPY);

				BITMAP bmpInfo;
				GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);
				WORD wPalSize, wBmpBits = bmpInfo.bmPlanes * bmpInfo.bmBitsPixel;
				if (wBmpBits <= 1)
				{
					wBmpBits = 1;  // monochrome image
					wPalSize = 2;
				}
				else if (wBmpBits <= 4)
				{
					wBmpBits = 4;  // palette-based 4 bpp image
					wPalSize = 16;
				}
				else if (wBmpBits <= 8)
				{
					wBmpBits = 8;  // palette-based 8 bpp image
					wPalSize = 256;
				}
				else
				{
					wBmpBits = 16; // force to 16 bpp image (don't allow 24 bpp)
					wPalSize = 0;  // don't use palette
				}

				m_dwBmpHdrSize = sizeof(BITMAPINFOHEADER) + wPalSize * sizeof(RGBQUAD);
				m_pBmpInfo = (PBITMAPINFO)new BYTE[m_dwBmpHdrSize];
				if (m_pBmpInfo)
				{
					ZeroMemory(m_pBmpInfo, m_dwBmpHdrSize);
					BITMAPINFOHEADER& bmpHdr = m_pBmpInfo->bmiHeader;
					bmpHdr.biSize = sizeof(bmpHdr);
					bmpHdr.biWidth = nWidth;
					bmpHdr.biHeight = nHeight;
					bmpHdr.biPlanes = 1;
					bmpHdr.biBitCount = wBmpBits;
					bmpHdr.biCompression = BI_RGB;

					// call GetDIBits with a NULL bits array, so it will calculate the biSizeImage field
					GetDIBits(hMemDC, hBitmap, 0, nHeight, NULL, m_pBmpInfo, DIB_RGB_COLORS);
					if (bmpHdr.biSizeImage == 0)
						bmpHdr.biSizeImage = (wBmpBits * nWidth + 31) / 32 * 4 * nHeight;
					m_dwBitsArraySize = bmpHdr.biSizeImage;
					m_pBitsArray = new BYTE[m_dwBitsArraySize];
					if (m_pBitsArray)
					{
						if (GetDIBits(hMemDC, hBitmap, 0, nHeight, m_pBitsArray, m_pBmpInfo, DIB_RGB_COLORS))
							bResult = TRUE;
					}
				}

				SelectBitmap(hMemDC, hbmpSafeBitmap);
				DeleteDC(hMemDC);
			}
			DeleteBitmap(hBitmap);
		}
		DeleteDC(hDisplayDC);
	}
	if (! bResult)
	{
		delete[] (PBYTE)m_pBitsArray;
		delete[] (PBYTE)m_pBmpInfo;
		ZeroMemory(this, sizeof(*this));
	}
}

/**
 * @param pszFileName - screen-shot file name.
 * @return true if screen-shot has been written successfully.
 */
BOOL CSymEngine::CScreenShot::WriteScreenShot(PCTSTR pszFileName)
{
	if (! m_pBmpInfo)
		return FALSE;
	HANDLE hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		BITMAPFILEHEADER bmpfh;
		ZeroMemory(&bmpfh, sizeof(bmpfh));
		bmpfh.bfType = 'MB';
		bmpfh.bfOffBits = sizeof(bmpfh) + m_dwBmpHdrSize;
		bmpfh.bfSize = bmpfh.bfOffBits + m_dwBitsArraySize;
		DWORD dwProcessedNumber;
		WriteFile(hFile, &bmpfh, sizeof(bmpfh), &dwProcessedNumber, NULL);
		WriteFile(hFile, m_pBmpInfo, m_dwBmpHdrSize, &dwProcessedNumber, NULL);
		WriteFile(hFile, m_pBitsArray, m_dwBitsArraySize, &dwProcessedNumber, NULL);
		CloseHandle(hFile);
		return TRUE;
	}
	else
		return FALSE;
}

/**
 * @return true if stack frame was adjusted.
 */
BOOL CSymEngine::AdjustExceptionStackFrame(void)
{
	if (m_hDbgHelpDll == NULL)
		return FALSE;
	BOOL bFoundThrowingFrame = FALSE;
	if (m_eExceptionType != WIN32_EXCEPTION)
	{
		STACKFRAME64 stFrame;
		if (! InitStackTrace(&stFrame))
			return FALSE;

		HANDLE hThread = GetCurrentThread();
		CONTEXT ctxSafe = m_StartExceptionContext;
		for (;;)
		{
			BOOL bResult = FStackWalk64(IMAGE_FILE_MACHINE_I386,
			                            m_hSymProcess,
			                            hThread,
			                            &stFrame,
			                            &m_StartExceptionContext,
			                            ReadProcessMemoryProc64,
			                            FSymFunctionTableAccess64,
			                            FSymGetModuleBase64,
			                            NULL);
			if (! bResult || ! stFrame.AddrFrame.Offset || bFoundThrowingFrame)
			{
				if (bFoundThrowingFrame)
					m_dwExceptionAddress = stFrame.AddrPC.Offset;
				else
					m_StartExceptionContext = ctxSafe;
				break;
			}
			BYTE arrSymBuffer[512];
			ZeroMemory(arrSymBuffer, sizeof(arrSymBuffer));
			PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)arrSymBuffer;
			pSymbol->SizeOfStruct = sizeof(*pSymbol);
			pSymbol->MaxNameLen = sizeof(arrSymBuffer) - sizeof(*pSymbol) + 1;
			DWORD64 dwDisplacement64;
			if (FSymFromAddr(m_hSymProcess, stFrame.AddrPC.Offset, &dwDisplacement64, pSymbol))
			{
				switch (m_eExceptionType)
				{
				case CPP_EXCEPTION:
					if (strcmp(pSymbol->Name, "_CxxThrowException") == 0 ||
						strcmp(pSymbol->Name, "CxxThrowException") == 0)
					{
						bFoundThrowingFrame = TRUE;
					}
					break;
#ifdef _MANAGED
				case NET_EXCEPTION:
					if (strcmp(pSymbol->Name, "BT_CallNetFilter") == 0)
					{
						bFoundThrowingFrame = TRUE;
					}
					break;
#endif
				case NO_EXCEPTION:
					if (strcmp(pSymbol->Name, "BT_MakeSnapshot") == 0)
					{
						bFoundThrowingFrame = TRUE;
					}
					break;
				}
			}
		}
	}
	return bFoundThrowingFrame;
}

/**
 * @param dwException - system exception code.
 * @return description of the exception.
 */
PCTSTR CSymEngine::ConvertExceptionCodeToString(DWORD dwException)
{
#define CONVERT_EXCEPTION_CODE_TO_STRING(exception) \
	case EXCEPTION_##exception: return _T(#exception)

	switch (dwException)
	{
	CONVERT_EXCEPTION_CODE_TO_STRING(ACCESS_VIOLATION);
	CONVERT_EXCEPTION_CODE_TO_STRING(DATATYPE_MISALIGNMENT);
	CONVERT_EXCEPTION_CODE_TO_STRING(BREAKPOINT);
	CONVERT_EXCEPTION_CODE_TO_STRING(SINGLE_STEP);
	CONVERT_EXCEPTION_CODE_TO_STRING(ARRAY_BOUNDS_EXCEEDED);
	CONVERT_EXCEPTION_CODE_TO_STRING(FLT_DENORMAL_OPERAND);
	CONVERT_EXCEPTION_CODE_TO_STRING(FLT_DIVIDE_BY_ZERO);
	CONVERT_EXCEPTION_CODE_TO_STRING(FLT_INEXACT_RESULT);
	CONVERT_EXCEPTION_CODE_TO_STRING(FLT_INVALID_OPERATION);
	CONVERT_EXCEPTION_CODE_TO_STRING(FLT_OVERFLOW);
	CONVERT_EXCEPTION_CODE_TO_STRING(FLT_STACK_CHECK);
	CONVERT_EXCEPTION_CODE_TO_STRING(FLT_UNDERFLOW);
	CONVERT_EXCEPTION_CODE_TO_STRING(INT_DIVIDE_BY_ZERO);
	CONVERT_EXCEPTION_CODE_TO_STRING(INT_OVERFLOW);
	CONVERT_EXCEPTION_CODE_TO_STRING(PRIV_INSTRUCTION);
	CONVERT_EXCEPTION_CODE_TO_STRING(IN_PAGE_ERROR);
	CONVERT_EXCEPTION_CODE_TO_STRING(ILLEGAL_INSTRUCTION);
	CONVERT_EXCEPTION_CODE_TO_STRING(NONCONTINUABLE_EXCEPTION);
	CONVERT_EXCEPTION_CODE_TO_STRING(STACK_OVERFLOW);
	CONVERT_EXCEPTION_CODE_TO_STRING(INVALID_DISPOSITION);
	CONVERT_EXCEPTION_CODE_TO_STRING(GUARD_PAGE);
	CONVERT_EXCEPTION_CODE_TO_STRING(INVALID_HANDLE);
	default: return NULL;
	}

#undef CONVERT_EXCEPTION_CODE_TO_STRING
}

/**
 * @param rErrorInfo - system error information.
 */
void CSymEngine::GetSysErrorInfo(CSysErrorInfo& rErrorInfo)
{
	if (m_dwLastError)
	{
		_stprintf_s(rErrorInfo.m_szErrorCode, countof(rErrorInfo.m_szErrorCode), _T("0x%08lX"), m_dwLastError);
		_ASSERTE(rErrorInfo.m_pszMessageBuffer == NULL);
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		              NULL, m_dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		              (PTSTR)&rErrorInfo.m_pszMessageBuffer, 0, NULL);
		if (rErrorInfo.m_pszMessageBuffer)
			TrimSpaces(rErrorInfo.m_pszMessageBuffer);
	}
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetSysErrorString(CUTF8EncStream& rEncStream)
{
	CSysErrorInfo ErrorInfo;
	GetSysErrorInfo(ErrorInfo);
	if (*ErrorInfo.m_szErrorCode)
	{
		rEncStream.WriteUTF8Bin(ErrorInfo.m_szErrorCode);
		if (ErrorInfo.m_pszMessageBuffer && *ErrorInfo.m_pszMessageBuffer)
		{
			rEncStream.WriteAscii(" - ");
			rEncStream.WriteUTF8Bin(ErrorInfo.m_pszMessageBuffer);
		}
	}
}

/**
 * @param rErrorInfo - COM error information.
 * @return true if error information was retrieved,
 */
BOOL CSymEngine::GetComErrorInfo(CComErrorInfo& rErrorInfo)
{
	if (! m_pErrorInfo)
		return FALSE;

	BSTR bstrDescription = NULL;
	HRESULT hRes = m_pErrorInfo->GetDescription(&bstrDescription);
	if (SUCCEEDED(hRes) && bstrDescription)
	{
		if (*bstrDescription)
			rErrorInfo.m_strDescription = bstrDescription;
		SysFreeString(bstrDescription);
		bstrDescription = NULL;
	}

	BSTR bstrHelpFile = NULL;
	hRes = m_pErrorInfo->GetHelpFile(&bstrHelpFile);
	if (SUCCEEDED(hRes) && bstrHelpFile)
	{
		if (*bstrHelpFile)
			rErrorInfo.m_strHelpFile = bstrHelpFile;
		SysFreeString(bstrHelpFile);
		bstrHelpFile = NULL;
	}

	BSTR bstrSource = NULL;
	hRes = m_pErrorInfo->GetSource(&bstrSource);
	if (SUCCEEDED(hRes) && bstrSource)
	{
		if (*bstrSource)
			rErrorInfo.m_strSource = bstrSource;
		SysFreeString(bstrSource);
		bstrSource = NULL;
	}

	GUID guid;
	hRes = m_pErrorInfo->GetGUID(&guid);
	if (SUCCEEDED(hRes))
	{
		OLECHAR szGuid[64];
		hRes = StringFromGUID2(guid, szGuid, countof(szGuid));
		if (SUCCEEDED(hRes))
			rErrorInfo.m_strGuid = szGuid;
	}

	return TRUE;
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetComErrorString(CUTF8EncStream& rEncStream)
{
	CComErrorInfo ErrorInfo;
	if (! GetComErrorInfo(ErrorInfo))
		return;

	static const CHAR szNewLine[] = "\r\n";
	static const CHAR szDescriptionMsg[] = "Description: ";
	static const CHAR szHelpFileMsg[] = "Help File:   ";
	static const CHAR szSourceMsg[] = "Source:      ";
	static const CHAR szGuidMsg[] = "GUID:        ";

	BOOL bNotEmpty = FALSE;

	if (! ErrorInfo.m_strDescription.IsEmpty())
	{
		bNotEmpty = TRUE;
		rEncStream.WriteAscii(szDescriptionMsg);
		rEncStream.WriteUTF8Bin(ErrorInfo.m_strDescription);
	}

	if (! ErrorInfo.m_strHelpFile.IsEmpty())
	{
		if (bNotEmpty)
			rEncStream.WriteAscii(szNewLine);
		else
			bNotEmpty = TRUE;
		rEncStream.WriteAscii(szHelpFileMsg);
		rEncStream.WriteUTF8Bin(ErrorInfo.m_strHelpFile);
	}

	if (! ErrorInfo.m_strSource.IsEmpty())
	{
		if (bNotEmpty)
			rEncStream.WriteAscii(szNewLine);
		else
			bNotEmpty = TRUE;
		rEncStream.WriteAscii(szSourceMsg);
		rEncStream.WriteUTF8Bin(ErrorInfo.m_strSource);
	}

	if (! ErrorInfo.m_strGuid.IsEmpty())
	{
		if (bNotEmpty)
			rEncStream.WriteAscii(szNewLine);
		else
			bNotEmpty = TRUE;
		rEncStream.WriteAscii(szGuidMsg);
		rEncStream.WriteUTF8Bin(ErrorInfo.m_strGuid);
	}
}

/**
 * @param rErrorInfo - error information.
 * @return true if error information was retrieved,
 */
BOOL CSymEngine::GetErrorInfo(CErrorInfo& rErrorInfo)
{
	if (m_hDbgHelpDll == NULL || m_eExceptionType == NO_EXCEPTION)
		return FALSE;
	GetModuleFileName(NULL, rErrorInfo.m_szProcessName, countof(rErrorInfo.m_szProcessName));
	PCTSTR pszProcessName = PathFindFileName(rErrorInfo.m_szProcessName);
	DWORD dwProcessName = _tcslen(pszProcessName);
	MoveMemory(rErrorInfo.m_szProcessName, pszProcessName, (dwProcessName + 1) * sizeof(TCHAR));
	_ultot_s(GetCurrentProcessId(), rErrorInfo.m_szProcessID, countof(rErrorInfo.m_szProcessID), 10);
	DWORD dwException;
	switch (m_eExceptionType)
	{
	case WIN32_EXCEPTION:
		dwException = m_pExceptionPointers != NULL ? m_pExceptionPointers->ExceptionRecord->ExceptionCode : 0;
		rErrorInfo.m_pszWhat = ConvertExceptionCodeToString(dwException);
		if (rErrorInfo.m_pszWhat == NULL)
			rErrorInfo.m_pszWhat = _T("UNKNOWN_ERROR");
		break;
	case CPP_EXCEPTION:
		rErrorInfo.m_pszWhat = _T("NATIVE_EXCEPTION");
		break;
#ifdef _MANAGED
	case NET_EXCEPTION:
		rErrorInfo.m_pszWhat = _T("MANAGED_EXCEPTION");
		break;
#endif
	}
	GetFirstStackTraceEntry(rErrorInfo);
	return TRUE;
}

/**
 * @param rStream - stream object.
 */
void CSymEngine::GetWin32ErrorString(CStrStream& rStream)
{
	CErrorInfo ErrorInfo;
	if (! GetErrorInfo(ErrorInfo))
		return;

	rStream << ErrorInfo.m_szProcessName;
	rStream << _T(" caused ");
	rStream << ErrorInfo.m_pszWhat;

	if (*ErrorInfo.m_szModule)
	{
		rStream << _T(" in module \"");
		rStream << ErrorInfo.m_szModule;
		rStream << _T('\"');
	}

	if (*ErrorInfo.m_szAddress)
	{
		rStream << _T(" at ");
		rStream << ErrorInfo.m_szAddress;
	}

	if (*ErrorInfo.m_szFunctionInfo)
	{
		rStream << _T(", ");
		rStream << ErrorInfo.m_szFunctionInfo;
	}

	if (*ErrorInfo.m_szSourceFile)
	{
		rStream << _T(" in \"");
		rStream << ErrorInfo.m_szSourceFile;
		rStream << _T('\"');

		if (*ErrorInfo.m_szLineInfo)
		{
			rStream << _T(", ");
			rStream << ErrorInfo.m_szLineInfo;
		}
	}
}

#ifdef _MANAGED

/**
 * @param rStream - stream object.
 */
void CSymEngine::GetNetErrorString(CStrStream& rStream)
{
	_ASSERTE(m_pNetStackTrace != NULL);
	if (m_pNetStackTrace != NULL)
		m_pNetStackTrace->GetErrorString(rStream);
	else
		rStream.Free();
}

/**
 * @param rStream - stream object.
 */
void CSymEngine::GetErrorString(CStrStream& rStream)
{
	if (m_pNetStackTrace != NULL)
		GetNetErrorString(rStream);
	else
		GetWin32ErrorString(rStream);
}

#endif

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetWin32ErrorString(CUTF8EncStream& rEncStream)
{
	CStrStream Stream(1024);
	GetWin32ErrorString(Stream);
	rEncStream.WriteUTF8Bin(Stream);
}

#ifdef _MANAGED

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetNetErrorString(CUTF8EncStream& rEncStream)
{
	CStrStream Stream(1024);
	GetNetErrorString(Stream);
	rEncStream.WriteUTF8Bin(Stream);
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetErrorString(CUTF8EncStream& rEncStream)
{
	CStrStream Stream(1024);
	GetErrorString(Stream);
	rEncStream.WriteUTF8Bin(Stream);
}

#endif

/**
 * @param rRegVals - registers values in string format.
 */
void CSymEngine::GetRegistersValues(CRegistersValues& rRegVals)
{
	_ASSERTE(m_pExceptionPointers != NULL);

#define CONVERT_REGISTER_VALUE_TO_STRING(reg, digits) \
	_stprintf_s(rRegVals.m_sz##reg, countof(rRegVals.m_sz##reg), _T("0x%0") _T(#digits) _T("lX"), m_pExceptionPointers->ContextRecord->reg);

	CONVERT_REGISTER_VALUE_TO_STRING(Eax, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(Ebx, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(Ecx, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(Edx, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(Esi, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(Edi, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(Esp, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(Ebp, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(Eip, 8);
	CONVERT_REGISTER_VALUE_TO_STRING(SegCs, 4);
	CONVERT_REGISTER_VALUE_TO_STRING(SegDs, 4);
	CONVERT_REGISTER_VALUE_TO_STRING(SegSs, 4);
	CONVERT_REGISTER_VALUE_TO_STRING(SegEs, 4);
	CONVERT_REGISTER_VALUE_TO_STRING(SegFs, 4);
	CONVERT_REGISTER_VALUE_TO_STRING(SegGs, 4);
	CONVERT_REGISTER_VALUE_TO_STRING(EFlags, 8);

#undef CONVERT_REGISTER_VALUE_TO_STRING
}

/**
 * @param pszRegString - buffer that receives registers values.
 * @param dwRegStringSize - size of buffer.
 */
void CSymEngine::GetRegistersString(PTSTR pszRegString, DWORD dwRegStringSize)
{
	_ASSERTE(m_pExceptionPointers != NULL);
	_stprintf_s(pszRegString, dwRegStringSize,
	           _T("EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X\r\n")
	           _T("ESI=%08X  EDI=%08X  FLG=%08X\r\n")
	           _T("EBP=%08X  ESP=%08X  EIP=%08X\r\n")
	           _T("CS=%04X  DS=%04X  SS=%04X  ES=%04X  FS=%04X  GS=%04X"),

	           m_pExceptionPointers->ContextRecord->Eax,
	           m_pExceptionPointers->ContextRecord->Ebx,
	           m_pExceptionPointers->ContextRecord->Ecx,
	           m_pExceptionPointers->ContextRecord->Edx,

	           m_pExceptionPointers->ContextRecord->Esi,
	           m_pExceptionPointers->ContextRecord->Edi,
	           m_pExceptionPointers->ContextRecord->EFlags,

	           m_pExceptionPointers->ContextRecord->Ebp,
	           m_pExceptionPointers->ContextRecord->Esp,
	           m_pExceptionPointers->ContextRecord->Eip,

	           m_pExceptionPointers->ContextRecord->SegCs,
	           m_pExceptionPointers->ContextRecord->SegDs,
	           m_pExceptionPointers->ContextRecord->SegSs,
	           m_pExceptionPointers->ContextRecord->SegEs,
	           m_pExceptionPointers->ContextRecord->SegFs,
	           m_pExceptionPointers->ContextRecord->SegGs);
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetRegistersString(CUTF8EncStream& rEncStream)
{
	TCHAR szRegString[256];
	GetRegistersString(szRegString, countof(szRegString));
	rEncStream.WriteUTF8Bin(szRegString);
}

/**
 * @param rCpusInfo - CPUs information.
 */
void CSymEngine::GetCpusInfo(CCpusInfo& rCpusInfo)
{
	static const TCHAR szIntel[] = _T("Intel");
	static const TCHAR szIA64[] = _T("IA-64");
	static const TCHAR szAMD64[] = _T("AMD-64");
	static const TCHAR szUnknown[] = _T("Unknown");

	rCpusInfo.m_pszCpuArch = szUnknown;
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if (g_bWinNT)
	{
		switch (si.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_INTEL:
			rCpusInfo.m_pszCpuArch = szIntel;
			break;
		case PROCESSOR_ARCHITECTURE_IA64:
			rCpusInfo.m_pszCpuArch = szIA64;
			break;
		case PROCESSOR_ARCHITECTURE_AMD64:
			rCpusInfo.m_pszCpuArch = szAMD64;
			break;
		default: // PROCESSOR_ARCHITECTURE_UNKNOWN
			rCpusInfo.m_pszCpuArch = szUnknown;
			break;
		}
	}
	else
		rCpusInfo.m_pszCpuArch = szIntel;
	rCpusInfo.m_dwNumCpus = si.dwNumberOfProcessors;
}

/**
 * @param dwCpuNum - CPU number.
 * @param rCpuInfo - CPU information.
 * @return true if cpu info was successfully retrieved.
 */
BOOL CSymEngine::GetCpuInfo(DWORD dwCpuNum, CCpuInfo& rCpuInfo)
{
	static const TCHAR szQuestion[] = _T("?");
	static const TCHAR szUnknown[] = _T("Unknown");

	_tcscpy_s(rCpuInfo.m_szCpuId, countof(rCpuInfo.m_szCpuId), szUnknown);
	_tcscpy_s(rCpuInfo.m_szCpuSpeed, countof(rCpuInfo.m_szCpuSpeed), szQuestion);
	_tcscpy_s(rCpuInfo.m_szCpuDescription, countof(rCpuInfo.m_szCpuId), szUnknown);

	BOOL bResult = TRUE;
	if (g_bWinNT)
	{
		TCHAR szCentralProcessorPath[MAX_PATH];
		_stprintf_s(szCentralProcessorPath, countof(szCentralProcessorPath), _T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%lu"), dwCpuNum);
		HKEY hKey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szCentralProcessorPath, 0l, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			DWORD dwValue, dwValueType, dwValueSize;
			dwValueSize = sizeof(dwValue);
			if (RegQueryValueEx(hKey, _T("~MHz"), NULL, &dwValueType, (PBYTE)&dwValue, &dwValueSize) == ERROR_SUCCESS && dwValueType == REG_DWORD)
				_ultot_s(dwValue, rCpuInfo.m_szCpuSpeed, countof(rCpuInfo.m_szCpuSpeed), 10);
			else
				*rCpuInfo.m_szCpuSpeed = _T('\0');
			dwValueSize = sizeof(rCpuInfo.m_szCpuDescription);
			if (RegQueryValueEx(hKey, _T("ProcessorNameString"), NULL, &dwValueType, (PBYTE)rCpuInfo.m_szCpuDescription, &dwValueSize) == ERROR_SUCCESS && dwValueType == REG_SZ)
				TrimSpaces(rCpuInfo.m_szCpuDescription);
			else
				*rCpuInfo.m_szCpuDescription = _T('\0');
			dwValueSize = sizeof(rCpuInfo.m_szCpuId);
			if (RegQueryValueEx(hKey, _T("Identifier"), NULL, &dwValueType, (PBYTE)rCpuInfo.m_szCpuId, &dwValueSize) == ERROR_SUCCESS && dwValueType == REG_SZ)
				TrimSpaces(rCpuInfo.m_szCpuId);
			else
				*rCpuInfo.m_szCpuId = _T('\0');
			RegCloseKey(hKey);
		}
		else
			bResult = FALSE;
	}
	return bResult;
}

/**
 * @param rStream - buffer that receives CPU information.
 */
void CSymEngine::GetCpuString(CStrStream& rStream)
{
	CCpusInfo CpusInfo;
	GetCpusInfo(CpusInfo);
	rStream << _T("Number of Processors:  ");
	TCHAR szTempBuf[16];
	_ultot_s(CpusInfo.m_dwNumCpus, szTempBuf, countof(szTempBuf), 10);
	rStream << szTempBuf;
	rStream << _T("\r\nProcessors Descriptions:");
	for (DWORD dwCpuNum = 0; dwCpuNum < CpusInfo.m_dwNumCpus; ++dwCpuNum)
	{
		CCpuInfo CpuInfo;
		if (GetCpuInfo(dwCpuNum, CpuInfo))
		{
			rStream << _T("\r\n");
			_ultot_s(dwCpuNum + 1, szTempBuf, countof(szTempBuf), 10);
			rStream << szTempBuf;
			rStream << _T(". ");
			rStream << CpuInfo.m_szCpuDescription;
		}
	}
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetCpuString(CUTF8EncStream& rEncStream)
{
	CStrStream Stream;
	GetCpuString(Stream);
	rEncStream.WriteUTF8Bin(Stream);
}

/**
 * @param rOsInfo - OS information.
 */
void CSymEngine::GetOsInfo(COsInfo& rOsInfo)
{
	static const TCHAR szUnknown[] = _T("Unknown");
	static const TCHAR szWindowsNT351[] = _T("Windows NT 3.51");
	static const TCHAR szWindowsNT40[] = _T("Windows NT 4.0");
	static const TCHAR szWindows95[] = _T("Windows 95");
	static const TCHAR szWindows98[] = _T("Windows 98");
	static const TCHAR szWindowsMe[] = _T("Windows Me");
	static const TCHAR szWindows2000[] = _T("Windows 2000");
	static const TCHAR szWindowsXP[] = _T("Windows XP");

	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);

	rOsInfo.m_pszWinVersion = szUnknown;
	switch (osvi.dwMajorVersion)
	{
	case 3:
		if (osvi.dwMinorVersion == 51 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
			rOsInfo.m_pszWinVersion = szWindowsNT351;
		break;
	case 4:
		switch (osvi.dwMinorVersion)
		{
		case 0:
			if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
				rOsInfo.m_pszWinVersion = szWindowsNT40;
			else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				rOsInfo.m_pszWinVersion = szWindows95;
			break;
		case 10:
			if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				rOsInfo.m_pszWinVersion = szWindows98;
			break;
		case 90:
			if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				rOsInfo.m_pszWinVersion = szWindowsMe;
			break;
		}
		break;
	case 5:
		switch (osvi.dwMinorVersion)
		{
		case 0:
			if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
				rOsInfo.m_pszWinVersion = szWindows2000;
			break;
		case 1:
			if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
				rOsInfo.m_pszWinVersion = szWindowsXP;
			break;
		}
		break;
	}

	_tcscpy_s(rOsInfo.m_szSPVersion, countof(rOsInfo.m_szSPVersion), osvi.szCSDVersion);
	_ultot_s(osvi.dwBuildNumber, rOsInfo.m_szBuildNumber, countof(rOsInfo.m_szBuildNumber), 10);

#ifdef _MANAGED
	NetThunks::GetNetVersion(rOsInfo.m_szNetVersion, countof(rOsInfo.m_szNetVersion));
#endif
}

/**
 * @param pszOSString - pointer to OS string.
 * @param dwOSStringSize - size of OS string buffer.
 */
void CSymEngine::GetOsString(PTSTR pszOSString, DWORD dwOSStringSize)
{
	COsInfo OsInfo;
	GetOsInfo(OsInfo);
#ifdef _MANAGED
	_stprintf_s(pszOSString, dwOSStringSize,
	            _T("OS Version:    %s %s\r\n")
	            _T("Build Number:  %s\r\n")
				_T("CLR Version:   %s"),
	            OsInfo.m_pszWinVersion,
	            OsInfo.m_szSPVersion,
	            OsInfo.m_szBuildNumber,
				OsInfo.m_szNetVersion);
#else
	_stprintf_s(pszOSString, dwOSStringSize,
	            _T("OS Version:    %s %s\r\n")
	            _T("Build Number:  %s"),
	            OsInfo.m_pszWinVersion,
	            OsInfo.m_szSPVersion,
	            OsInfo.m_szBuildNumber);
#endif
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetOsString(CUTF8EncStream& rEncStream)
{
	TCHAR szOSString[256];
	GetOsString(szOSString, countof(szOSString));
	rEncStream.WriteUTF8Bin(szOSString);
}

/**
 * @param rMemInfo - memory information.
 */
void CSymEngine::GetMemInfo(CMemInfo& rMemInfo)
{
	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	_ultot_s(ms.dwMemoryLoad, rMemInfo.m_szMemoryLoad, countof(rMemInfo.m_szMemoryLoad), 10);
	_ultot_s(ms.dwTotalPhys, rMemInfo.m_szTotalPhys, countof(rMemInfo.m_szTotalPhys), 10);
	_ultot_s(ms.dwAvailPhys, rMemInfo.m_szAvailPhys, countof(rMemInfo.m_szAvailPhys), 10);
	_ultot_s(ms.dwTotalPageFile, rMemInfo.m_szTotalPageFile, countof(rMemInfo.m_szTotalPageFile), 10);
	_ultot_s(ms.dwAvailPageFile, rMemInfo.m_szAvailPageFile, countof(rMemInfo.m_szAvailPageFile), 10);
}

/**
 * @param pszMemString - pointer to memory status string.
 * @param dwMemStringSize - size of memory status string buffer.
 */
void CSymEngine::GetMemString(PTSTR pszMemString, DWORD dwMemStringSize)
{
	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	_stprintf_s(pszMemString, dwMemStringSize,
	            _T("Current Memory Load:         %lu%%\r\n")
	            _T("Total Physical Memory:       %lu MB\r\n")
	            _T("Available Physical Memory:   %lu MB\r\n")
	            _T("Total Page File Memory:      %lu MB\r\n")
	            _T("Available Page File Memory:  %lu MB"),
	            ms.dwMemoryLoad,
	            ms.dwTotalPhys / (1024 * 1024),
	            ms.dwAvailPhys / (1024 * 1024),
	            ms.dwTotalPageFile / (1024 * 1024),
	            ms.dwAvailPageFile / (1024 * 1024));
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetMemString(CUTF8EncStream& rEncStream)
{
	TCHAR szMemString[256];
	GetMemString(szMemString, countof(szMemString));
	rEncStream.WriteUTF8Bin(szMemString);
}

/**
 * @param pDestination - destination address.
 * @param pSource - source address.
 * @param dwSize - number of bytes to copy.
 */
void CSymEngine::SafeCopy(PVOID pDestination, PVOID pSource, DWORD dwSize)
{
	__try {
		for (DWORD dwBytePos = 0; dwBytePos < dwSize; ++dwBytePos)
			((PBYTE)pDestination)[dwBytePos] = ((PBYTE)pSource)[dwBytePos];
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		// do nothing - just ignore the exception...
	}
}

/**
 * @param pStackFrame - pointer to the exception stack frame.
 * @return true if thread context was successfully resolved.
 */
BOOL CSymEngine::InitStackTrace(LPSTACKFRAME64 pStackFrame)
{
	if (m_pExceptionPointers != NULL)
	{
		// It's necessary to make a copy of CONTEXT structure because StackWalk64() changes it.
		// However this line of code doesn't work on Windows 9x:
		//   m_StartExceptionContext = *m_pExceptionPointers->ContextRecord;
		// I think the size of CONTEXT structure was increased since Windows 9x
		// and m_pExceptionPointers->ContextRecord points to smaller buffer.
		// Windows 9x considers such copy attempt as access violation, so it's safer
		// to define custom copy function and catch there all errors. It's not the
		// problem to copy smaller chunk to larger buffer, because Windows 9x uses
		// only properly copied part of larger memory block.
		SafeCopy(&m_StartExceptionContext, m_pExceptionPointers->ContextRecord, sizeof(m_StartExceptionContext));
	}
	else
	{
/*
		HANDLE hThread = GetCurrentThread();
		m_StartExceptionContext.ContextFlags = CONTEXT_FULL;
		if (! GetThreadContext(hThread, &m_StartExceptionContext))
			return FALSE;
*/
		__asm
		{
			push eax
			push edi

			push es
			push ecx

			// EDI = &m_StartExceptionContext
			mov edi, [this]
			add edi, [m_StartExceptionContext]
			// ES = DS
			push ds
			pop es
			// ECX = sizeof(m_StartExceptionContext)
			mov ecx, size m_StartExceptionContext
			// EAX = 0
			xor eax, eax
			// store EDI
			push edi
			// ZeroMemory(&m_StartExceptionContext, sizeof(m_StartExceptionContext))
			cld
			rep stosb
			// restore EDI
			pop edi

			pop ecx
			pop es

			// fill m_StartExceptionContext
			mov dword ptr [edi] CONTEXT.ContextFlags, CONTEXT_FULL
			mov dword ptr [edi] CONTEXT.Eax, eax
			mov dword ptr [edi] CONTEXT.Ecx, ecx
			mov dword ptr [edi] CONTEXT.Edx, edx
			mov dword ptr [edi] CONTEXT.Ebx, ebx
			mov dword ptr [edi] CONTEXT.Esi, esi
			mov dword ptr [edi] CONTEXT.Edi, edi
			mov word ptr [edi] CONTEXT.SegSs, ss
			mov word ptr [edi] CONTEXT.SegCs, cs
			mov word ptr [edi] CONTEXT.SegDs, ds
			mov word ptr [edi] CONTEXT.SegEs, es
			mov word ptr [edi] CONTEXT.SegFs, fs
			mov word ptr [edi] CONTEXT.SegGs, gs
			// m_StartExceptionContext.EFlags = flags
			pushfd
			pop [edi] CONTEXT.EFlags
			// extract caller's EBP, IP (return address) and ESP
			mov eax, [ebp]
			mov dword ptr [edi] CONTEXT.Ebp, eax
			mov eax, [ebp + 4]
			mov dword ptr [edi] CONTEXT.Eip, eax
			lea eax, [ebp + 8]
			mov dword ptr [edi] CONTEXT.Esp, eax

			pop edi
			pop eax
		}
	}

	ZeroMemory(pStackFrame, sizeof(*pStackFrame));
	pStackFrame->AddrPC.Mode = AddrModeFlat;
	pStackFrame->AddrPC.Offset = m_StartExceptionContext.Eip;
	pStackFrame->AddrPC.Segment = (WORD)m_StartExceptionContext.SegCs;
	pStackFrame->AddrStack.Mode = AddrModeFlat;
	pStackFrame->AddrStack.Offset = m_StartExceptionContext.Esp;
	pStackFrame->AddrStack.Segment = (WORD)m_StartExceptionContext.SegSs;
	pStackFrame->AddrFrame.Mode = AddrModeFlat;
	pStackFrame->AddrFrame.Offset = m_StartExceptionContext.Ebp;
	pStackFrame->AddrFrame.Segment = (WORD)m_StartExceptionContext.SegEs;
	return TRUE;
}

/**
 * @param hThread - handle of examined thread; pass NULL if you want to get exception stack trace.
 * @return true if thread context was successfully resolved.
 */
BOOL CSymEngine::InitStackTrace(HANDLE hThread)
{
	if (! hThread)
	{
		hThread = GetCurrentThread();
		// It's necessary to make a copy of CONTEXT structure because StackWalk64() changes it.
		// However this line of code doesn't work on Windows 9x:
		//   m_swContext.m_context = *m_pExceptionPointers->ContextRecord;
		// I think the size of CONTEXT structure was increased since Windows 9x
		// and m_pExceptionPointers->ContextRecord points to smaller buffer.
		// Windows 9x considers such copy attempt as access violation, so it's safer
		// to define custom copy function and catch there all errors. It's not the
		// problem to copy smaller chunk to larger buffer, because Windows 9x uses
		// only properly copied part of larger memory block.
		if (m_eExceptionType == WIN32_EXCEPTION)
		{
			_ASSERTE(m_pExceptionPointers != NULL);
			SafeCopy(&m_swContext.m_context, m_pExceptionPointers->ContextRecord, sizeof(m_swContext.m_context));
		}
		else
			m_swContext.m_context = m_StartExceptionContext;
	}
	else
	{
		m_swContext.m_context.ContextFlags = CONTEXT_FULL;
		if (! GetThreadContext(hThread, &m_swContext.m_context))
			return FALSE;
	}
	m_swContext.m_hThread = hThread;
	ZeroMemory(&m_swContext.m_stFrame, sizeof(m_swContext.m_stFrame));
	m_swContext.m_stFrame.AddrPC.Mode = AddrModeFlat;
	m_swContext.m_stFrame.AddrPC.Offset = m_swContext.m_context.Eip;
	m_swContext.m_stFrame.AddrStack.Mode = AddrModeFlat;
	m_swContext.m_stFrame.AddrStack.Offset = m_swContext.m_context.Esp;
	m_swContext.m_stFrame.AddrFrame.Mode = AddrModeFlat;
	m_swContext.m_stFrame.AddrFrame.Offset = m_swContext.m_context.Ebp;
	m_dwFrameCount = 0;
	return TRUE;
}

/**
 * @param rEncStream - UTF-8 encoder object.
 * @return true if thread context was successfully resolved.
 */
BOOL CSymEngine::GetNextWin32StackTraceString(CUTF8EncStream& rEncStream)
{
	CStackTraceEntry Entry;
	if (! GetNextStackTraceEntry(Entry))
		return FALSE;

	if (*Entry.m_szModule)
	{
		rEncStream.WriteByte('\"');
		rEncStream.WriteUTF8Bin(Entry.m_szModule);
		rEncStream.WriteByte('\"');
	}

	if (*Entry.m_szAddress)
	{
		rEncStream.WriteAscii(" at ");
		rEncStream.WriteUTF8Bin(Entry.m_szAddress);
	}

	if (*Entry.m_szFunctionInfo)
	{
		rEncStream.WriteAscii(", ");
		rEncStream.WriteUTF8Bin(Entry.m_szFunctionInfo);
	}

	if (*Entry.m_szSourceFile)
	{
		rEncStream.WriteAscii(" in \"");
		rEncStream.WriteUTF8Bin(Entry.m_szSourceFile);
		rEncStream.WriteByte('\"');
	}

	if (*Entry.m_szLineInfo)
	{
		rEncStream.WriteAscii(", ");
		rEncStream.WriteUTF8Bin(Entry.m_szLineInfo);
	}

	return TRUE;
}

/**
 * @param rStream - stream object.
 */
void CSymEngine::GetEnvironmentStrings(CStrStream& rStream)
{
	TCHAR* pchEnvironment = ::GetEnvironmentStrings();
	if (pchEnvironment)
	{
		const TCHAR* pchEnvironmentPair = pchEnvironment;
		while (*pchEnvironmentPair)
		{
			DWORD dwItemLength = _tcslen(pchEnvironmentPair);
			rStream << pchEnvironmentPair << _T("\r\n");
			pchEnvironmentPair += dwItemLength + 1;
		}
		FreeEnvironmentStrings(pchEnvironment);
	}
}

/**
 * @param rXmlWriter - XML writer.
 */
void CSymEngine::GetEnvironmentStrings(CXmlWriter& rXmlWriter)
{
	rXmlWriter.WriteStartElement(_T("environment")); // <environment>
	TCHAR* pchEnvironment = ::GetEnvironmentStrings();
	if (pchEnvironment)
	{
		const TCHAR* pchEnvironmentPair = pchEnvironment;
		while (*pchEnvironmentPair)
		{
			const TCHAR* pchEnd;
			const TCHAR* pchEquation = _tcschr(pchEnvironmentPair, _T('='));
			if (pchEquation)
			{
				pchEnd = _tcschr(pchEquation, _T('\0'));
				TCHAR szVariableName[MAX_PATH];
				_tcsncpy_s(szVariableName, countof(szVariableName), pchEnvironmentPair, pchEquation - pchEnvironmentPair);
				rXmlWriter.WriteStartElement(_T("variable")); // <variable>
				 rXmlWriter.WriteElementString(_T("name"), szVariableName); // <name>...</name>
				 rXmlWriter.WriteElementString(_T("value"), pchEquation + 1); // <value>...</value>
				rXmlWriter.WriteEndElement(); // </variable>
			}
			else
				pchEnd = _tcschr(pchEnvironmentPair, _T('\0'));
			pchEnvironmentPair = pchEnd + 1;
		}
		FreeEnvironmentStrings(pchEnvironment);
	}
	rXmlWriter.WriteEndElement(); // </environment>
}

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetEnvironmentStrings(CUTF8EncStream& rEncStream)
{
	CStrStream Stream(8 * 1024);
	GetEnvironmentStrings(Stream);
	rEncStream.WriteUTF8Bin(Stream);
}

#ifdef _MANAGED

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetAssemblyList(CUTF8EncStream& rEncStream)
{
	static const CHAR szAppDomainMsg[] = "\r\nApplication Domain: ";
	static const CHAR szAssembliesMsg[] = ", Assemblies:\r\n";
	static const CHAR szAppDomainIDMsg[] = ", ID: ";
	static const CHAR szAssemblyNameMsg[] = "\r\nAssembly Name: ";
	static const CHAR szAssemblyVersionMsg[] = "\r\nAssembly Version: ";
	static const CHAR szFileVersionMsg[] = "\r\nFile Version: ";
	static const CHAR szCodeBaseMsg[] = "\r\nCode Base: ";
	static const CHAR szDividerMsg[] = "----------------------------------------\r\n";
	static const CHAR szNewLine[] = "\r\n";

	DWORD dwAppDomainID;
	WCHAR szAppDomainName[MAX_PATH];
	NetThunks::GetAppDomainInfo(dwAppDomainID, szAppDomainName, countof(szAppDomainName));
	CHAR szAppDomainID[32];
	_ultoa_s(dwAppDomainID, szAppDomainID, countof(szAppDomainID), 10);

	rEncStream.WriteAscii(szAppDomainMsg);
	rEncStream.WriteUTF8Bin(szAppDomainName);
	rEncStream.WriteAscii(szAppDomainIDMsg);
	rEncStream.WriteAscii(szAppDomainID);

	CNetAssemblies NetAssemblies;
	CNetAssemblies::CAssemblyInfo AssemblyInfo;
	if (NetAssemblies.GetFirstAssembly(AssemblyInfo))
	{
		rEncStream.WriteAscii(szAssembliesMsg);
		rEncStream.WriteAscii(szDividerMsg);
		do
		{
			rEncStream.WriteAscii(szAssemblyNameMsg);
			rEncStream.WriteUTF8Bin(AssemblyInfo.m_szName);

			rEncStream.WriteAscii(szAssemblyVersionMsg);
			rEncStream.WriteUTF8Bin(AssemblyInfo.m_szVersion);

			rEncStream.WriteAscii(szFileVersionMsg);
			rEncStream.WriteUTF8Bin(AssemblyInfo.m_szFileVersion);

			rEncStream.WriteAscii(szCodeBaseMsg);
			rEncStream.WriteUTF8Bin(AssemblyInfo.m_szCodeBase);

			rEncStream.WriteAscii(szNewLine);
		}
		while (NetAssemblies.GetNextAssembly(AssemblyInfo));
	}
	else
		rEncStream.WriteAscii(szNewLine);
}

/**
 * @param rXmlWriter - XML writer.
 */
void CSymEngine::GetAssemblyList(CXmlWriter& rXmlWriter)
{
	DWORD dwAppDomainID;
	WCHAR szAppDomainName[MAX_PATH];
	NetThunks::GetAppDomainInfo(dwAppDomainID, szAppDomainName, countof(szAppDomainName));
	TCHAR szAppDomainID[32];
	_ultot_s(dwAppDomainID, szAppDomainID, countof(szAppDomainID), 10);

	rXmlWriter.WriteStartElement(_T("app-domain")); // <app-domain>
	 rXmlWriter.WriteElementString(_T("name"), szAppDomainName); // <name>...</name>
	 rXmlWriter.WriteElementString(_T("id"), szAppDomainID); // <id>...</id>
	 rXmlWriter.WriteStartElement(_T("assemblies")); // <assemblies>

	 CNetAssemblies NetAssemblies;
	 CNetAssemblies::CAssemblyInfo AssemblyInfo;
	 if (NetAssemblies.GetFirstAssembly(AssemblyInfo))
	 {
		 do
		 {
			 rXmlWriter.WriteStartElement(_T("assembly")); // <assembly>
			  rXmlWriter.WriteElementString(_T("name"), AssemblyInfo.m_szName); // <name>...</name>
			  rXmlWriter.WriteElementString(_T("version"), AssemblyInfo.m_szVersion); // <version>...</version>
			  rXmlWriter.WriteElementString(_T("file-version"), AssemblyInfo.m_szFileVersion); // <file-version>...</file-version>
			  rXmlWriter.WriteElementString(_T("code-base"), AssemblyInfo.m_szCodeBase); // <code-base>...</code-base>
			 rXmlWriter.WriteEndElement(); // </assembly>
		 }
		 while (NetAssemblies.GetNextAssembly(AssemblyInfo));
	 }

	 rXmlWriter.WriteEndElement(); // </assemblies>
	rXmlWriter.WriteEndElement(); // </app-domain>
}

#endif

/**
 * @param rEncStream - UTF-8 encoder object.
 * @param pEnumProcess - pointer to the process enumerator;
 * @param rProcEntry - process entry.
 */
void CSymEngine::GetModuleList(CUTF8EncStream& rEncStream, CEnumProcess* pEnumProcess, CEnumProcess::CProcessEntry& rProcEntry)
{
	_ASSERTE(pEnumProcess != NULL);

	static const CHAR szProcessMsg[] = "\r\nProcess: ";
	static const CHAR szModulesMsg[] = ", Modules:\r\n";
	static const CHAR szProcessIDMsg[] = ", PID: ";
	static const CHAR szBaseMsg[] = ", Base: ";
	static const CHAR szSizeMsg[] = ", Size: ";
	static const CHAR szDividerMsg[] = "----------------------------------------\r\n";
	static const CHAR szNewLine[] = "\r\n";

	CHAR szTempBuf[64];
	_ultoa_s(rProcEntry.m_dwProcessID, szTempBuf, countof(szTempBuf), 10);

	rEncStream.WriteAscii(szProcessMsg);
	rEncStream.WriteUTF8Bin(rProcEntry.m_szProcessName);
	rEncStream.WriteAscii(szProcessIDMsg);
	rEncStream.WriteAscii(szTempBuf);

	CEnumProcess::CModuleEntry ModuleEntry;
	if (pEnumProcess->GetModuleFirst(rProcEntry.m_dwProcessID, ModuleEntry))
	{
		rEncStream.WriteAscii(szModulesMsg);
		rEncStream.WriteAscii(szDividerMsg);
		do
		{
			rEncStream.WriteUTF8Bin(ModuleEntry.m_szModuleName);
			TCHAR szVersionString[64];
			if (GetVersionString(ModuleEntry.m_szModuleName, szVersionString, countof(szVersionString)))
			{
				rEncStream.WriteAscii(" (");
				rEncStream.WriteUTF8Bin(szVersionString);
				rEncStream.WriteByte(_T(')'));
			}
			rEncStream.WriteAscii(szBaseMsg);
			sprintf_s(szTempBuf, countof(szTempBuf), "%08lX", (DWORD)ModuleEntry.m_pLoadBase);
			rEncStream.WriteAscii(szTempBuf);
			rEncStream.WriteAscii(szSizeMsg);
			sprintf_s(szTempBuf, countof(szTempBuf), "%08lX", (DWORD)ModuleEntry.m_dwModuleSize);
			rEncStream.WriteAscii(szTempBuf);
			rEncStream.WriteAscii(szNewLine);
		}
		while (pEnumProcess->GetModuleNext(rProcEntry.m_dwProcessID, ModuleEntry));
	}
	else
		rEncStream.WriteAscii(szNewLine);
}

/**
 * @param rXmlWriter - XML writer.
 * @param pEnumProcess - pointer to the process enumerator;
 * @param rProcEntry - process entry.
 */
void CSymEngine::GetModuleList(CXmlWriter& rXmlWriter, CEnumProcess* pEnumProcess, CEnumProcess::CProcessEntry& rProcEntry)
{
	_ASSERTE(pEnumProcess != NULL);

	rXmlWriter.WriteStartElement(_T("process")); // <process>
	 rXmlWriter.WriteElementString(_T("name"), rProcEntry.m_szProcessName); // <name>...</name>
	 TCHAR szTempBuf[64];
	 _ultot_s(rProcEntry.m_dwProcessID, szTempBuf, countof(szTempBuf), 10);
	 rXmlWriter.WriteElementString(_T("id"), szTempBuf); // <id>...</id>
	 rXmlWriter.WriteStartElement(_T("modules")); // <modules>

	 CEnumProcess::CModuleEntry ModuleEntry;
	 BOOL bContinue = pEnumProcess->GetModuleFirst(rProcEntry.m_dwProcessID, ModuleEntry);
	 while (bContinue)
	 {
		 rXmlWriter.WriteStartElement(_T("module")); // <module>
		  rXmlWriter.WriteElementString(_T("name"), ModuleEntry.m_szModuleName); // <name>...</name>
		  TCHAR szVersionString[64];
		  if (! GetVersionString(ModuleEntry.m_szModuleName, szVersionString, countof(szVersionString)))
			  *szVersionString = _T('\0');
		  rXmlWriter.WriteElementString(_T("version"), szVersionString); // <version>...</version>
		  _stprintf_s(szTempBuf, countof(szTempBuf), _T("0x%08lX"), (DWORD)ModuleEntry.m_pLoadBase);
		  rXmlWriter.WriteElementString(_T("base"), szTempBuf); // <base>...</base>
		  _stprintf_s(szTempBuf, countof(szTempBuf), _T("0x%08lX"), (DWORD)ModuleEntry.m_dwModuleSize);
		  rXmlWriter.WriteElementString(_T("size"), szTempBuf); // <size>...</size>
		 rXmlWriter.WriteEndElement(); // </module>
		 bContinue = pEnumProcess->GetModuleNext(rProcEntry.m_dwProcessID, ModuleEntry);
	 }

	 rXmlWriter.WriteEndElement(); // </modules>
	rXmlWriter.WriteEndElement(); // </process>
}

/**
 * @param rXmlWriter - XML writer.
 * @param pEnumProcess - pointer to the process enumerator;
 */
void CSymEngine::GetProcessList(CXmlWriter& rXmlWriter, CEnumProcess* pEnumProcess)
{
	rXmlWriter.WriteStartElement(_T("processes")); // <processes>
	_ASSERTE(pEnumProcess != NULL);
	CEnumProcess::CProcessEntry ProcEntry;
	if (g_dwFlags & BTF_LISTPROCESSES)
	{
		if (pEnumProcess->GetProcessFirst(ProcEntry))
		{
			do
				GetModuleList(rXmlWriter, pEnumProcess, ProcEntry);
			while (pEnumProcess->GetProcessNext(ProcEntry));
			g_pEnumProc->Close();
		}
	}
	else
	{
		pEnumProcess->GetCurrentProcess(ProcEntry);
		GetModuleList(rXmlWriter, pEnumProcess, ProcEntry);
	}
	rXmlWriter.WriteEndElement(); // </processes>
}

/**
 * @param rEncStream - UTF-8 encoder object.
 * @param pEnumProcess - pointer to the process enumerator;
 */
void CSymEngine::GetProcessList(CUTF8EncStream& rEncStream, CEnumProcess* pEnumProcess)
{
	_ASSERTE(pEnumProcess != NULL);
	CEnumProcess::CProcessEntry ProcEntry;
	if (g_dwFlags & BTF_LISTPROCESSES)
	{
		if (pEnumProcess->GetProcessFirst(ProcEntry))
		{
			do
				GetModuleList(rEncStream, pEnumProcess, ProcEntry);
			while (pEnumProcess->GetProcessNext(ProcEntry));
			g_pEnumProc->Close();
		}
	}
	else
	{
		pEnumProcess->GetCurrentProcess(ProcEntry);
		GetModuleList(rEncStream, pEnumProcess, ProcEntry);
	}
}
/**
 * @param rEncStream - UTF-8 encoder object.
 * @param dwThreadID - thread ID.
 * @param hThread - thread handle.
 * @param pszThreadStatus - thread status.
 */
void CSymEngine::GetWin32StackTrace(CUTF8EncStream& rEncStream, DWORD dwThreadID, HANDLE hThread, PCSTR pszThreadStatus)
{
	static const CHAR szTraceMsg[] = "\r\nStack Trace: ";
	static const CHAR szThreadIDMsg[] = ", TID: ";
	static const CHAR szDividerMsg[] = "----------------------------------------\r\n";
	static const CHAR szNewLine[] = "\r\n";

	CHAR szThreadID[32];
	_ultoa_s(dwThreadID, szThreadID, countof(szThreadID), 10);

	rEncStream.WriteAscii(szTraceMsg);
	rEncStream.WriteAscii(pszThreadStatus);
	rEncStream.WriteAscii(szThreadIDMsg);
	rEncStream.WriteAscii(szThreadID);
	rEncStream.WriteAscii(szNewLine);
	rEncStream.WriteAscii(szDividerMsg);

	BOOL bContinue = GetFirstWin32StackTraceString(rEncStream, hThread);
	while (bContinue)
	{
		rEncStream.WriteAscii(szNewLine);
		bContinue = GetNextWin32StackTraceString(rEncStream);
	}
}

/**
 * @param rXmlWriter - XML writer.
 * @param dwThreadID - thread ID.
 * @param hThread - thread handle.
 * @param pszThreadStatus - thread status.
 */
void CSymEngine::GetWin32StackTrace(CXmlWriter& rXmlWriter, DWORD dwThreadID, HANDLE hThread, PCTSTR pszThreadStatus)
{
	rXmlWriter.WriteStartElement(_T("thread")); // <thread>

	 TCHAR szThreadID[32];
	 _ultot_s(dwThreadID, szThreadID, countof(szThreadID), 10);
	 rXmlWriter.WriteElementString(_T("id"), szThreadID); // <id>...</id>
	 rXmlWriter.WriteElementString(_T("status"), pszThreadStatus); // <status>...</status>
	 rXmlWriter.WriteStartElement(_T("stack")); // <stack>

	  CStackTraceEntry Entry;
	  BOOL bContinue = GetFirstStackTraceEntry(Entry, hThread);
	  while (bContinue)
	  {
		  rXmlWriter.WriteStartElement(_T("frame")); // <frame>
		   rXmlWriter.WriteElementString(_T("module"), Entry.m_szModule); // <module>...</module>
		   rXmlWriter.WriteElementString(_T("address"), Entry.m_szAddress); // <address>...</address>
		   rXmlWriter.WriteStartElement(_T("function")); // <function>
		    rXmlWriter.WriteElementString(_T("name"), Entry.m_szFunctionName); // <name>...</name>
		    rXmlWriter.WriteElementString(_T("offset"), Entry.m_szFunctionOffset); // <offset>...</offset>
		   rXmlWriter.WriteEndElement(); // </function>
		   rXmlWriter.WriteElementString(_T("file"), Entry.m_szSourceFile); // <file>...</file>
		   rXmlWriter.WriteStartElement(_T("line")); // <line>
		    rXmlWriter.WriteElementString(_T("number"), Entry.m_szLineNumber); // <number>...</number>
		   rXmlWriter.WriteElementString(_T("offset"), Entry.m_szLineOffset); // <offset>...</offset>
		   rXmlWriter.WriteEndElement(); // </line>
		  rXmlWriter.WriteEndElement(); // </frame>
		  bContinue = GetNextStackTraceEntry(Entry);
	  }

	 rXmlWriter.WriteEndElement(); // </stack>
	rXmlWriter.WriteEndElement(); // </thread>
}

#ifdef _MANAGED

/**
 * @param rEncStream - UTF-8 encoder object.
 */
void CSymEngine::GetNetStackTrace(CUTF8EncStream& rEncStream)
{
	_ASSERTE(m_pNetStackTrace != NULL);

	static const CHAR szTraceMsg[] = "\r\nCLR Stack Trace: ";
	static const CHAR szInterruptedStateMsg[] = "Interrupted Thread";
	static const CHAR szActiveStateMsg[] = "Active Thread";
	static const CHAR szThreadIDMsg[] = ", TID: ";
	static const CHAR szThreadNameMsg[] = ", Name: ";
	static const CHAR szDividerMsg[] = "----------------------------------------\r\n";
	static const CHAR szNewLine[] = "\r\n";
	PCSTR pszThreadStatus = m_pExceptionPointers != NULL ? szInterruptedStateMsg : szActiveStateMsg;

	DWORD dwThreadID;
	WCHAR szThreadName[128];
	NetThunks::GetThreadInfo(dwThreadID, szThreadName, countof(szThreadName));
	CHAR szThreadID[32];
	_ultoa_s(dwThreadID, szThreadID, countof(szThreadID), 10);

	rEncStream.WriteAscii(szTraceMsg);
	rEncStream.WriteAscii(pszThreadStatus);
	if (*szThreadName)
	{
		rEncStream.WriteAscii(szThreadNameMsg);
		rEncStream.WriteUTF8Bin(szThreadName);
	}
	rEncStream.WriteAscii(szThreadIDMsg);
	rEncStream.WriteAscii(szThreadID);
	rEncStream.WriteAscii(szNewLine);
	rEncStream.WriteAscii(szDividerMsg);

	bool bContinue = m_pNetStackTrace->GetFirstStackTraceString(rEncStream);
	while (bContinue)
	{
		rEncStream.WriteAscii(szNewLine);
		bContinue = m_pNetStackTrace->GetNextStackTraceString(rEncStream);
	}
}

/**
 * @param rXmlWriter - XML writer.
 */
void CSymEngine::GetNetStackTrace(CXmlWriter& rXmlWriter)
{
	_ASSERTE(m_pNetStackTrace != NULL);

	static const TCHAR szInterruptedState[] = _T("interrupted");
	static const TCHAR szActiveState[] = _T("active");
	PCTSTR pszThreadStatus = m_pExceptionPointers != NULL ? szInterruptedState : szActiveState;

	rXmlWriter.WriteStartElement(_T("clr-thread")); // <clr-thread>

	DWORD dwThreadID;
	WCHAR szThreadName[128];
	NetThunks::GetThreadInfo(dwThreadID, szThreadName, countof(szThreadName));
	 rXmlWriter.WriteElementString(_T("name"), szThreadName); // <name>...</name>
	 TCHAR szThreadID[32];
	 _ultot_s(dwThreadID, szThreadID, countof(szThreadID), 10);
	 rXmlWriter.WriteElementString(_T("id"), szThreadID); // <id>...</id>
	 rXmlWriter.WriteElementString(_T("status"), pszThreadStatus); // <status>...</status>
	 rXmlWriter.WriteStartElement(_T("stack")); // <stack>

	  CNetStackTrace::CNetStackTraceEntry Entry;
	  BOOL bContinue = m_pNetStackTrace->GetFirstStackTraceEntry(Entry);
	  while (bContinue)
	  {
		  rXmlWriter.WriteStartElement(_T("frame")); // <frame>
		   rXmlWriter.WriteElementString(_T("assembly"), Entry.m_szAssembly); // <assembly>...</assembly>
		   rXmlWriter.WriteElementString(_T("native-offset"), Entry.m_szNativeOffset); // <native-offset>...</native-offset>
		   rXmlWriter.WriteElementString(_T("il-offset"), Entry.m_szILOffset); // <il-offset>...</il-offset>
		   rXmlWriter.WriteElementString(_T("type"), Entry.m_szType); // <type>...</type>
		   rXmlWriter.WriteElementString(_T("method"), Entry.m_szMethod); // <method>...</method>
		   rXmlWriter.WriteElementString(_T("file"), Entry.m_szSourceFile); // <file>...</file>
		   rXmlWriter.WriteElementString(_T("line"), Entry.m_szLineNumber); // <line>...</line>
		   rXmlWriter.WriteElementString(_T("column"), Entry.m_szColumnNumber); // <column>...</column>
		  rXmlWriter.WriteEndElement(); // </frame>
		  bContinue = m_pNetStackTrace->GetNextStackTraceEntry(Entry);
	  }

	 rXmlWriter.WriteEndElement(); // </stack>
	rXmlWriter.WriteEndElement(); // </clr-thread>
}

#endif

/**
 * @param rEncStream - UTF-8 encoder object.
 * @param pEnumProcess - pointer to the process enumerator;
 */
void CSymEngine::GetWin32ThreadsList(CUTF8EncStream& rEncStream, CEnumProcess* pEnumProcess)
{
	_ASSERTE(pEnumProcess != NULL);
	if (! FOpenThread)
		return;

	static const CHAR szSuspendedStateMsg[] = "Suspended Thread";
	static const CHAR szRunningStateMsg[] = "Running Thread";
	static const CHAR szActiveStateMsg[] = "Active Thread";

	DWORD dwCurrentThreadID = GetCurrentThreadId();
	DWORD dwCurrentProcessID = GetCurrentProcessId();
	CEnumProcess::CThreadEntry thr;

	if (pEnumProcess->GetThreadFirst(dwCurrentProcessID, thr))
	{
		do
		{
			bool bActiveThread = thr.m_dwThreadID == dwCurrentThreadID;
			if (m_pExceptionPointers != NULL && bActiveThread)
				continue;

			PCSTR pszThreadStatus;
			if (bActiveThread)
			{
				pszThreadStatus = szActiveStateMsg;
				GetWin32StackTrace(rEncStream, thr.m_dwThreadID, NULL, pszThreadStatus);
			}
			else
			{
				HANDLE hThread = FOpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, FALSE, thr.m_dwThreadID);
				if (hThread == NULL)
					continue;
				pszThreadStatus = SuspendThread(hThread) ? szSuspendedStateMsg : szRunningStateMsg;
				GetWin32StackTrace(rEncStream, thr.m_dwThreadID, hThread, pszThreadStatus);
				if (m_pExceptionPointers == NULL || thr.m_dwThreadID == g_dwExceptionThreadID)
					ResumeThread(hThread);
				CloseHandle(hThread);
			}
		}
		while (pEnumProcess->GetThreadNext(dwCurrentProcessID, thr));
	}
}

/**
 * @param rXmlWriter - XML writer.
 * @param pEnumProcess - pointer to the process enumerator;
 */
void CSymEngine::GetWin32ThreadsList(CXmlWriter& rXmlWriter, CEnumProcess* pEnumProcess)
{
	_ASSERTE(pEnumProcess != NULL);
	if (! FOpenThread)
		return;

	static const TCHAR szSuspendedState[] = _T("suspended");
	static const TCHAR szRunningState[] = _T("running");
	static const TCHAR szActiveState[] = _T("active");

	DWORD dwCurrentThreadID = GetCurrentThreadId();
	DWORD dwCurrentProcessID = GetCurrentProcessId();
	CEnumProcess::CThreadEntry thr;

	if (pEnumProcess->GetThreadFirst(dwCurrentProcessID, thr))
	{
		do
		{
			bool bActiveThread = thr.m_dwThreadID == dwCurrentThreadID;
			if (m_pExceptionPointers != NULL && bActiveThread)
				continue;
			PCTSTR pszThreadStatus;
			if (bActiveThread)
			{
				pszThreadStatus = szActiveState;
				GetWin32StackTrace(rXmlWriter, thr.m_dwThreadID, NULL, pszThreadStatus);
			}
			else
			{
				HANDLE hThread = FOpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, FALSE, thr.m_dwThreadID);
				if (hThread == NULL)
					continue;
				pszThreadStatus = SuspendThread(hThread) ? szSuspendedState : szRunningState;
				GetWin32StackTrace(rXmlWriter, thr.m_dwThreadID, hThread, pszThreadStatus);
				if (m_pExceptionPointers == NULL || thr.m_dwThreadID == g_dwExceptionThreadID)
					ResumeThread(hThread);
				CloseHandle(hThread);
			}
		}
		while (pEnumProcess->GetThreadNext(dwCurrentProcessID, thr));
	}
}

/**
 * @param pszModuleName - module file name.
 * @param pszVersionString - place to store version information.
 * @param dwVersionStringSize - size of version string buffer.
 * @return true if operation was completed successfully.
 */
BOOL CSymEngine::GetVersionString(PCTSTR pszModuleName, PTSTR pszVersionString, DWORD dwVersionStringSize)
{
	if (dwVersionStringSize == 0)
		return FALSE;
	*pszVersionString = _T('\0');
	DWORD dwSize, dwHandle;
	dwSize = GetFileVersionInfoSize((PTSTR)pszModuleName, &dwHandle);
	if (dwSize == 0)
		return FALSE;
	UINT uLength;
	VS_FIXEDFILEINFO* pFileVerInfo;
	PBYTE pVersionInfo = new BYTE[dwSize];
	if (pVersionInfo == NULL)
		return FALSE;
	BOOL bResult = FALSE;
	if (GetFileVersionInfo((PTSTR)pszModuleName, dwHandle, dwSize, pVersionInfo) &&
		VerQueryValue(pVersionInfo, _T("\\"), (PVOID*)&pFileVerInfo, &uLength))
	{
		_ASSERTE(uLength == sizeof(*pFileVerInfo));
		_stprintf_s(pszVersionString, dwVersionStringSize,
			        _T("%lu.%lu.%lu.%lu"),
			        HIWORD(pFileVerInfo->dwFileVersionMS),
			        LOWORD(pFileVerInfo->dwFileVersionMS),
			        HIWORD(pFileVerInfo->dwFileVersionLS),
			        LOWORD(pFileVerInfo->dwFileVersionLS));
		bResult = TRUE;
	}
	delete[] pVersionInfo;
	return bResult;
}

/**
 * @param pszDateTime - pointer to the date-time buffer.
 * @param dwDateTimeSize - date-time buffer size.
 */
void CSymEngine::GetDateTime(PTSTR pszDateTime, DWORD dwDateTimeSize)
{
	_ASSERTE(dwDateTimeSize > 0);
	// Computers use different locales, so it's desirable to use universal date and time format.
	const LCID lcidEnglishUS = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
	DWORD dwOutputSize = GetDateFormat(lcidEnglishUS, LOCALE_USE_CP_ACP | DATE_LONGDATE | LOCALE_NOUSEROVERRIDE, &m_DateTime, NULL, pszDateTime, dwDateTimeSize);
	if (dwOutputSize < dwDateTimeSize)
		pszDateTime[dwOutputSize - 1] = _T(' ');
	GetTimeFormat(lcidEnglishUS, LOCALE_USE_CP_ACP | LOCALE_NOUSEROVERRIDE, &m_DateTime, NULL, pszDateTime + dwOutputSize, dwDateTimeSize - dwOutputSize);
}

/**
 * @param pszTimeStamp - pointer to the time-stamp buffer.
 * @param dwTimeStampSize - time-stamp buffer size.
 */
void CSymEngine::GetTimeStamp(PTSTR pszTimeStamp, DWORD dwTimeStampSize)
{
	FILETIME ftime;
	SystemTimeToFileTime(&m_DateTime, &ftime);
	ULARGE_INTEGER uint;
	uint.LowPart = ftime.dwLowDateTime;
	uint.HighPart = ftime.dwHighDateTime;
	_ui64tot_s(uint.QuadPart, pszTimeStamp, dwTimeStampSize, 10);
}

/**
 * @param rEncStream - UTF-8 encoder object.
 * @param pEnumProcess - pointer to the process enumerator;
 * pass NULL pointer if you want to skip process and module list.
 */
void CSymEngine::GetErrorLog(CUTF8EncStream& rEncStream, CEnumProcess* pEnumProcess)
{
	static const CHAR szAppMsg[] = "Application: ";
	static const CHAR szVersionMsg[] = "Version: ";
	static const CHAR szComputerNameMsg[] = "Computer: ";
	static const CHAR szUserNameMsg[] = "User: ";
	static const CHAR szDateTimeMsg[] = "Date: ";
	static const CHAR szDividerMsg[] = "----------------------------------------\r\n";
	static const CHAR szErrorMsg[] = "\r\n\r\nError Reason:\r\n";
	static const CHAR szCommandLineMsg[] = "\r\nCommand Line:\r\n";
	static const CHAR szCurrentDirMsg[] = "\r\n\r\nCurrent Directory:\r\n";
	static const CHAR szEnvironmentMsg[] = "\r\n\r\nEnvironment Variables:\r\n";
	static const CHAR szInterruptedStateMsg[] = "Interrupted Thread";
	static const CHAR szUserMsg[] = "\r\n\r\nUser Message:\r\n";
	static const CHAR szSysErrorMsg[] = "\r\n\r\nSystem Error:\r\n";
	static const CHAR szCOMErrorMsg[] = "\r\n\r\nCOM Error:\r\n";
	static const CHAR szRegistersMsg[] = "\r\n\r\nRegisters:\r\n";
	static const CHAR szCpuMsg[] = "\r\n\r\nCPU:\r\n";
	static const CHAR szOSMsg[] = "\r\n\r\nOperating System:\r\n";
	static const CHAR szMemMsg[] = "\r\n\r\nMemory Usage:\r\n";
	static const CHAR szNewLine[] = "\r\n";

#ifdef _MANAGED
	bool bNativeInfo = m_pNetStackTrace == NULL || (g_dwFlags & BTF_NATIVEINFO) != 0;
#endif

	if (*g_szAppName)
	{
		rEncStream.WriteAscii(szAppMsg);
		rEncStream.WriteUTF8Bin(g_szAppName);
		rEncStream.WriteAscii(szNewLine);
	}

	if (*g_szAppVersion)
	{
		rEncStream.WriteAscii(szVersionMsg);
		rEncStream.WriteUTF8Bin(g_szAppVersion);
		rEncStream.WriteAscii(szNewLine);
	}

	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = countof(szComputerName);
	if (GetComputerName(szComputerName, &dwSize))
	{
		rEncStream.WriteAscii(szComputerNameMsg);
		rEncStream.WriteUTF8Bin(szComputerName);
		rEncStream.WriteAscii(szNewLine);
	}

	TCHAR szUserName[UNLEN + 1];
	dwSize = countof(szUserName);
	if (GetUserName(szUserName, &dwSize))
	{
		rEncStream.WriteAscii(szUserNameMsg);
		rEncStream.WriteUTF8Bin(szUserName);
		rEncStream.WriteAscii(szNewLine);
	}

	TCHAR szDateTime[64];
	GetDateTime(szDateTime, countof(szDateTime));
	rEncStream.WriteAscii(szDateTimeMsg);
	rEncStream.WriteUTF8Bin(szDateTime);

	if (m_pExceptionPointers != NULL)
	{
		rEncStream.WriteAscii(szErrorMsg);
		rEncStream.WriteAscii(szDividerMsg);
		GetErrorString(rEncStream);
	}

	if (! g_strUserMessage.IsEmpty())
	{
		rEncStream.WriteAscii(szUserMsg);
		rEncStream.WriteAscii(szDividerMsg);
		rEncStream.WriteUTF8Bin(g_strUserMessage);
	}

	// Temporary encoder object.
	CMemStream MemStream(1024);
	CUTF8EncStream EncStreamTmp(&MemStream);

	EncStreamTmp.Reset();
	GetSysErrorString(EncStreamTmp);
	if (MemStream.GetLength())
	{
		MemStream.SetPosition(0, FILE_BEGIN);
		rEncStream.WriteAscii(szSysErrorMsg);
		rEncStream.WriteAscii(szDividerMsg);
		rEncStream.Write(EncStreamTmp);
	}

	EncStreamTmp.Reset();
	GetComErrorString(EncStreamTmp);
	if (MemStream.GetLength())
	{
		MemStream.SetPosition(0, FILE_BEGIN);
		rEncStream.WriteAscii(szCOMErrorMsg);
		rEncStream.WriteAscii(szDividerMsg);
		rEncStream.Write(EncStreamTmp);
	}

	if (m_pExceptionPointers != NULL
#ifdef _MANAGED
		&& bNativeInfo
#endif
		)
	{
		rEncStream.WriteAscii(szRegistersMsg);
		rEncStream.WriteAscii(szDividerMsg);
		GetRegistersString(rEncStream);
	}

	rEncStream.WriteAscii(szCpuMsg);
	rEncStream.WriteAscii(szDividerMsg);
	GetCpuString(rEncStream);

	rEncStream.WriteAscii(szOSMsg);
	rEncStream.WriteAscii(szDividerMsg);
	GetOsString(rEncStream);

	rEncStream.WriteAscii(szMemMsg);
	rEncStream.WriteAscii(szDividerMsg);
	GetMemString(rEncStream);

#ifdef _MANAGED
	if (m_pNetStackTrace != NULL)
	{
		rEncStream.WriteAscii(szNewLine);
		GetNetStackTrace(rEncStream);
		GetNetThreadsList(rEncStream);
	}

	if (bNativeInfo)
	{
#endif
		if (m_pExceptionPointers != NULL)
		{
			DWORD dwCurrentThreadID = GetCurrentThreadId();
			GetWin32StackTrace(rEncStream, dwCurrentThreadID, NULL, szInterruptedStateMsg);
		}
		if (pEnumProcess != NULL)
			GetWin32ThreadsList(rEncStream, pEnumProcess);
#ifdef _MANAGED
	}
#endif

	PCTSTR pszCommandLine = GetCommandLine();
	_ASSERTE(pszCommandLine != NULL);
	rEncStream.WriteAscii(szCommandLineMsg);
	rEncStream.WriteAscii(szDividerMsg);
	rEncStream.WriteUTF8Bin(pszCommandLine);

	TCHAR szCurrentDirectory[MAX_PATH];
	if (GetCurrentDirectory(countof(szCurrentDirectory), szCurrentDirectory) > 0)
	{
		rEncStream.WriteAscii(szCurrentDirMsg);
		rEncStream.WriteAscii(szDividerMsg);
		rEncStream.WriteUTF8Bin(szCurrentDirectory);
	}

	rEncStream.WriteAscii(szEnvironmentMsg);
	rEncStream.WriteAscii(szDividerMsg);
	GetEnvironmentStrings(rEncStream);

#ifdef _MANAGED
	GetAssemblyList(rEncStream);
	if (bNativeInfo)
	{
#endif
		if (pEnumProcess != NULL)
			GetProcessList(rEncStream, pEnumProcess);
#ifdef _MANAGED
	}
#endif
}

/**
 * @param rXmlWriter - XML writer object.
 */
void CSymEngine::GetErrorReason(CXmlWriter& rXmlWriter)
{
	_ASSERTE(m_pExceptionPointers != NULL);
#ifdef _MANAGED
	if (m_pNetStackTrace != NULL)
	{
		CNetStackTrace::CNetErrorInfo ErrorInfo;
		m_pNetStackTrace->GetErrorInfo(ErrorInfo);

		rXmlWriter.WriteStartElement(_T("error")); // <error>
		 rXmlWriter.WriteElementString(_T("what"), _T("MANAGED_EXCEPTION")); // <what>...</what>
		 rXmlWriter.WriteStartElement(_T("exception")); // <exception>
		  rXmlWriter.WriteElementString(_T("type"), ErrorInfo.m_szException); // <type>...</type>
		  rXmlWriter.WriteElementString(_T("message"), ErrorInfo.m_szMessage); // <message>...</message>
		 rXmlWriter.WriteEndElement(); // </exception>
		 rXmlWriter.WriteStartElement(_T("process")); // <process>
		  rXmlWriter.WriteElementString(_T("name"), ErrorInfo.m_szProcessName); // <name>...</name>
		  rXmlWriter.WriteElementString(_T("id"), ErrorInfo.m_szProcessID); // <id>...</id>
		 rXmlWriter.WriteEndElement(); // </process>
		 rXmlWriter.WriteStartElement(_T("app-domain")); // <app-domain>
		  rXmlWriter.WriteElementString(_T("name"), ErrorInfo.m_szAppDomainName); // <name>...</name>
		  rXmlWriter.WriteElementString(_T("id"), ErrorInfo.m_szAppDomainID); // <id>...</id>
		 rXmlWriter.WriteEndElement(); // </app-domain>
		 rXmlWriter.WriteElementString(_T("assembly"), ErrorInfo.m_szAssembly); // <assembly>...</assembly>
		 rXmlWriter.WriteElementString(_T("native-offset"), ErrorInfo.m_szNativeOffset); // <native-offset>...</native-offset>
		 rXmlWriter.WriteElementString(_T("il-offset"), ErrorInfo.m_szILOffset); // <il-offset>...</il-offset>
		 rXmlWriter.WriteElementString(_T("type"), ErrorInfo.m_szType); // <type>...</type>
		 rXmlWriter.WriteElementString(_T("method"), ErrorInfo.m_szMethod); // <method>...</method>
		 rXmlWriter.WriteElementString(_T("file"), ErrorInfo.m_szSourceFile); // <file>...</file>
		 rXmlWriter.WriteElementString(_T("line"), ErrorInfo.m_szLineNumber); // <line>...</line>
		 rXmlWriter.WriteElementString(_T("column"), ErrorInfo.m_szColumnNumber); // <column>...</column>
		rXmlWriter.WriteEndElement(); // </error>
	}
	else
	{
#endif
		CErrorInfo ErrorInfo;
		GetErrorInfo(ErrorInfo);

		rXmlWriter.WriteStartElement(_T("error")); // <error>
		 rXmlWriter.WriteElementString(_T("what"), ErrorInfo.m_pszWhat); // <what>...</what>
		 rXmlWriter.WriteStartElement(_T("process")); // <process>
		  rXmlWriter.WriteElementString(_T("name"), ErrorInfo.m_szProcessName); // <name>...</name>
		  rXmlWriter.WriteElementString(_T("id"), ErrorInfo.m_szProcessID); // <id>...</id>
		 rXmlWriter.WriteEndElement(); // </process>
		 rXmlWriter.WriteElementString(_T("module"), ErrorInfo.m_szModule); // <module>...</module>
		 rXmlWriter.WriteElementString(_T("address"), ErrorInfo.m_szAddress); // <address>...</address>
		 rXmlWriter.WriteStartElement(_T("function")); // <function>
		  rXmlWriter.WriteElementString(_T("name"), ErrorInfo.m_szFunctionName); // <name>...</name>
		  rXmlWriter.WriteElementString(_T("offset"), ErrorInfo.m_szFunctionOffset); // <offset>...</offset>
		 rXmlWriter.WriteEndElement(); // </function>
		 rXmlWriter.WriteElementString(_T("file"), ErrorInfo.m_szSourceFile); // <file>...</file>
		 rXmlWriter.WriteStartElement(_T("line")); // <line>
		  rXmlWriter.WriteElementString(_T("number"), ErrorInfo.m_szLineNumber); // <number>...</number>
		  rXmlWriter.WriteElementString(_T("offset"), ErrorInfo.m_szLineOffset); // <offset>...</offset>
		 rXmlWriter.WriteEndElement(); // </line>
		rXmlWriter.WriteEndElement(); // </error>
#ifdef _MANAGED
	}
#endif
}

/**
 * @param rXmlWriter - XML writer object.
 */
void CSymEngine::GetRegistersInfo(CXmlWriter& rXmlWriter)
{
	_ASSERTE(m_pExceptionPointers != NULL);
	CRegistersValues RegVals;
	GetRegistersValues(RegVals);
	rXmlWriter.WriteStartElement(_T("registers")); // <registers>
	 rXmlWriter.WriteElementString(_T("eax"), RegVals.m_szEax); // <eax>...</eax>
	 rXmlWriter.WriteElementString(_T("ebx"), RegVals.m_szEbx); // <ebx>...</ebx>
	 rXmlWriter.WriteElementString(_T("ecx"), RegVals.m_szEcx); // <ecx>...</ecx>
	 rXmlWriter.WriteElementString(_T("edx"), RegVals.m_szEdx); // <edx>...</edx>
	 rXmlWriter.WriteElementString(_T("esi"), RegVals.m_szEsi); // <esi>...</esi>
	 rXmlWriter.WriteElementString(_T("edi"), RegVals.m_szEdi); // <edi>...</edi>
	 rXmlWriter.WriteElementString(_T("esp"), RegVals.m_szEsp); // <esp>...</esp>
	 rXmlWriter.WriteElementString(_T("ebp"), RegVals.m_szEbp); // <ebp>...</ebp>
	 rXmlWriter.WriteElementString(_T("eip"), RegVals.m_szEip); // <eip>...</eip>
	 rXmlWriter.WriteElementString(_T("cs"), RegVals.m_szSegCs); // <cs>...</cs>
	 rXmlWriter.WriteElementString(_T("ds"), RegVals.m_szSegDs); // <ds>...</ds>
	 rXmlWriter.WriteElementString(_T("ss"), RegVals.m_szSegSs); // <ss>...</ss>
	 rXmlWriter.WriteElementString(_T("es"), RegVals.m_szSegEs); // <es>...</es>
	 rXmlWriter.WriteElementString(_T("fs"), RegVals.m_szSegFs); // <fs>...</fs>
	 rXmlWriter.WriteElementString(_T("gs"), RegVals.m_szSegGs); // <gs>...</gs>
	 rXmlWriter.WriteElementString(_T("eflags"), RegVals.m_szEFlags); // <eflags>...</eflags>
	rXmlWriter.WriteEndElement(); // </registers>
}

/**
 * @param rXmlWriter - XML writer object.
 */
void CSymEngine::GetSysErrorInfo(CXmlWriter& rXmlWriter)
{
	CSysErrorInfo SysErrorInfo;
	GetSysErrorInfo(SysErrorInfo);
	rXmlWriter.WriteStartElement(_T("syserror")); // <syserror>
	 rXmlWriter.WriteElementString(_T("code"), SysErrorInfo.m_szErrorCode); // <code>...</code>
	 PCTSTR pszMessageBuffer = SysErrorInfo.m_pszMessageBuffer ? SysErrorInfo.m_pszMessageBuffer : _T("");
	 rXmlWriter.WriteElementString(_T("description"), pszMessageBuffer); // <description>...</description>
	rXmlWriter.WriteEndElement(); // </syserror>
}

/**
 * @param rXmlWriter - XML writer object.
 */
void CSymEngine::GetComErrorInfo(CXmlWriter& rXmlWriter)
{
	CComErrorInfo ComErrorInfo;
	GetComErrorInfo(ComErrorInfo);
	rXmlWriter.WriteStartElement(_T("comerror")); // <comerror>
	 rXmlWriter.WriteElementString(_T("description"), ComErrorInfo.m_strDescription); // <description>...</description>
	 rXmlWriter.WriteElementString(_T("helpfile"), ComErrorInfo.m_strHelpFile); // <helpfile>...</helpfile>
	 rXmlWriter.WriteElementString(_T("source"), ComErrorInfo.m_strSource); // <source>...</source>
	 rXmlWriter.WriteElementString(_T("guid"), ComErrorInfo.m_strGuid); // <guid>...</guid>
	rXmlWriter.WriteEndElement(); // </comerror>
}

/**
 * @param rXmlWriter - XML writer object.
 */
void CSymEngine::GetCpusInfo(CXmlWriter& rXmlWriter)
{
	CCpusInfo CpusInfo;
	GetCpusInfo(CpusInfo);
	rXmlWriter.WriteStartElement(_T("cpus")); // <cpus>
	 TCHAR szNumCpus[16];
	 _ultot_s(CpusInfo.m_dwNumCpus, szNumCpus, countof(szNumCpus), 10);
	 rXmlWriter.WriteElementString(_T("number"), szNumCpus); // <number>...</number>
	 rXmlWriter.WriteElementString(_T("architecture"), CpusInfo.m_pszCpuArch); // <architecture>...</architecture>
	 for (DWORD dwCpuNum = 0; dwCpuNum < CpusInfo.m_dwNumCpus; ++dwCpuNum)
	 {
	 	CCpuInfo CpuInfo;
	 	if (GetCpuInfo(dwCpuNum, CpuInfo))
	 	{
	 		rXmlWriter.WriteStartElement(_T("cpu")); // <cpu>
	 		 rXmlWriter.WriteElementString(_T("id"), CpuInfo.m_szCpuId); // <id>...</id>
	 		 rXmlWriter.WriteElementString(_T("speed"), CpuInfo.m_szCpuSpeed); // <speed>...</speed>
	 		 rXmlWriter.WriteElementString(_T("description"), CpuInfo.m_szCpuDescription); // <description>...</description>
	 		rXmlWriter.WriteEndElement(); // </cpu>
	 	}
	 }
	rXmlWriter.WriteEndElement(); // </cpus>
}

/**
 * @param rXmlWriter - XML writer object.
 */
void CSymEngine::GetOsInfo(CXmlWriter& rXmlWriter)
{
	COsInfo OsInfo;
	GetOsInfo(OsInfo);
	rXmlWriter.WriteStartElement(_T("os")); // <os>
	 rXmlWriter.WriteElementString(_T("version"), OsInfo.m_pszWinVersion); // <version>...</version>
	 rXmlWriter.WriteElementString(_T("spack"), OsInfo.m_szSPVersion); // <spack>...</spack>
	 rXmlWriter.WriteElementString(_T("build"), OsInfo.m_szBuildNumber); // <build>...</build>
#ifdef _MANAGED
	 rXmlWriter.WriteElementString(_T("clr-version"), OsInfo.m_szNetVersion); // <clr-version>...</clr-version>
#endif
	rXmlWriter.WriteEndElement(); // </os>
}

/**
 * @param rXmlWriter - XML writer object.
 */
void CSymEngine::GetMemInfo(CXmlWriter& rXmlWriter)
{
	CMemInfo MemInfo;
	GetMemInfo(MemInfo);
	rXmlWriter.WriteStartElement(_T("memory")); // <memory>
	 rXmlWriter.WriteElementString(_T("load"), MemInfo.m_szMemoryLoad); // <load>...</load>
	 rXmlWriter.WriteElementString(_T("totalphys"), MemInfo.m_szTotalPhys); // <totalphys>...</totalphys>
	 rXmlWriter.WriteElementString(_T("availphys"), MemInfo.m_szAvailPhys); // <availphys>...</availphys>
	 rXmlWriter.WriteElementString(_T("totalpage"), MemInfo.m_szTotalPageFile); // <totalpage>...</totalpage>
	 rXmlWriter.WriteElementString(_T("availpage"), MemInfo.m_szAvailPageFile); // <availpage>...</availpage>
	rXmlWriter.WriteEndElement(); // </memory>
}

/**
 * @param rXmlWriter - XML writer object.
 * @param pEnumProcess - pointer to the process enumerator;
 * pass NULL pointer if you want to skip process and module list.
 */
void CSymEngine::GetErrorLog(CXmlWriter& rXmlWriter, CEnumProcess* pEnumProcess)
{
	static const TCHAR szInterruptedState[] = _T("interrupted");

#ifdef _MANAGED
	bool bNativeInfo = m_pNetStackTrace == NULL || (g_dwFlags & BTF_NATIVEINFO) != 0;
#endif

	rXmlWriter.SetIndentation(_T(' '), 2);
	rXmlWriter.WriteStartDocument();
	 TCHAR szDateTime[64];
	 GetDateTime(szDateTime, countof(szDateTime));
	 TCHAR szHeader[256];
	 _stprintf_s(szHeader, countof(szHeader), _T("\r\n")
	             _T(" This %s was automatically generated\r\n")
				 _T(" by ") BUGTRAP_TITLE _T(" on %s\r\n"),
				 m_pExceptionPointers != NULL ? _T("error report") : _T("snapshot"), szDateTime);
	 rXmlWriter.WriteComment(szHeader); // <!--...-->

	 rXmlWriter.WriteStartElement(_T("report")); // <report>
	  rXmlWriter.WriteAttributeString(_T("version"), _T("1"));
#ifdef _MANAGED
	  rXmlWriter.WriteElementString(_T("platform"), _T(".NET")); // <platform>...</platform>
#else
	  rXmlWriter.WriteElementString(_T("platform"), _T("Win32")); // <platform>...</platform>
#endif
	  rXmlWriter.WriteElementString(_T("application"), g_szAppName); // <application>...</application>
	  rXmlWriter.WriteElementString(_T("version"), g_szAppVersion); // <version>...</version>

	  TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	  DWORD dwSize = countof(szComputerName);
	  if (! GetComputerName(szComputerName, &dwSize))
		  *szComputerName = _T('\0');
	  rXmlWriter.WriteElementString(_T("computer"), szComputerName); // <computer>...</computer>

	  TCHAR szUserName[UNLEN + 1];
	  dwSize = countof(szUserName);
	  if (! GetUserName(szUserName, &dwSize))
		  *szUserName = _T('\0');
	  rXmlWriter.WriteElementString(_T("user"), szUserName); // <user>...</user>

	  TCHAR szTimeStamp[64];
	  GetTimeStamp(szTimeStamp, countof(szTimeStamp));
	  rXmlWriter.WriteElementString(_T("timestamp"), szTimeStamp); // <timestamp>...</timestamp>

	  if (m_pExceptionPointers != NULL)
		GetErrorReason(rXmlWriter);

	  rXmlWriter.WriteElementString(_T("usermsg"), g_strUserMessage); // <usermsg>...</usermsg>

	  GetSysErrorInfo(rXmlWriter);
	  GetComErrorInfo(rXmlWriter);
	  if (m_pExceptionPointers != NULL
#ifdef _MANAGED
		&& bNativeInfo
#endif
		)
	  {
		  GetRegistersInfo(rXmlWriter);
	  }
	  GetCpusInfo(rXmlWriter);
	  GetOsInfo(rXmlWriter);
	  GetMemInfo(rXmlWriter);

#ifdef _MANAGED
	  if (m_pNetStackTrace != NULL)
	  {
		  rXmlWriter.WriteStartElement(_T("clr-threads")); // <clr-threads>
		   GetNetStackTrace(rXmlWriter);
		   GetNetThreadsList(rXmlWriter);
		  rXmlWriter.WriteEndElement(); // </clr-threads>
	  }

	  if (bNativeInfo)
	  {
#endif
		  rXmlWriter.WriteStartElement(_T("threads")); // <threads>
		   if (m_pExceptionPointers != NULL)
		   {
			   DWORD dwCurrentThreadID = GetCurrentThreadId();
			   GetWin32StackTrace(rXmlWriter, dwCurrentThreadID, NULL, szInterruptedState);
		   }
		   if (pEnumProcess != NULL)
			   GetWin32ThreadsList(rXmlWriter, pEnumProcess);
		  rXmlWriter.WriteEndElement(); // </threads>
#ifdef _MANAGED
	  }
#endif

	  PCTSTR pszCommandLine = GetCommandLine();
	  _ASSERTE(pszCommandLine != NULL);
	  rXmlWriter.WriteElementString(_T("cmdline"), pszCommandLine); // <cmdline>...</cmdline>

	  TCHAR szCurrentDirectory[MAX_PATH];
	  if (GetCurrentDirectory(countof(szCurrentDirectory), szCurrentDirectory) <= 0)
		  *szCurrentDirectory = _T('\0');
	  rXmlWriter.WriteElementString(_T("curdir"), szCurrentDirectory); // <curdir>...</curdir>
	  GetEnvironmentStrings(rXmlWriter);

#ifdef _MANAGED
	  GetAssemblyList(rXmlWriter);
	  if (bNativeInfo)
	  {
		  if (pEnumProcess != NULL)
			  GetProcessList(rXmlWriter, pEnumProcess);
	  }
#endif

	 rXmlWriter.WriteEndElement(); // </report>
	rXmlWriter.WriteEndDocument();
}

/**
 * @param hZipFile - zip archive handle.
 * @param pszFilePath - name of added file.
 * @param pszFileName - file name stored in archive.
 * @return true if file was successfully added.
 */
BOOL CSymEngine::AddFileToArchive(zipFile hZipFile, PCTSTR pszFilePath, PCTSTR pszFileName)
{
	PCSTR pszFileNameA;
#ifdef _UNICODE
	CHAR szFileNameA[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, szFileNameA, countof(szFileNameA), NULL, NULL);
	pszFileNameA = szFileNameA;
#else
	pszFileNameA = pszFileName;
#endif
	BOOL bResult = FALSE;
	HANDLE hFile = CreateFile(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		DWORD dwBufferSize = min(dwFileSize, g_dwMaxBufferSize);
		PBYTE pFileBuffer = new BYTE[dwBufferSize];
		if (pFileBuffer)
		{
			if (zipOpenNewFileInZip(hZipFile, pszFileNameA, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION) == Z_OK)
			{
				bResult = TRUE;
				for (;;)
				{
					DWORD dwProcessedNumber = 0;
					bResult = ReadFile(hFile, pFileBuffer, dwBufferSize, &dwProcessedNumber, NULL);
					if (! bResult || dwProcessedNumber == 0)
						break;
					bResult = zipWriteInFileInZip(hZipFile, pFileBuffer, dwProcessedNumber) == Z_OK;
					if (! bResult)
						break;
				}
				if (zipCloseFileInZip(hZipFile) != Z_OK)
					bResult = FALSE;
			}
			delete[] pFileBuffer;
		}
		CloseHandle(hFile);
	}
	else
	{
		DWORD dwLastError = GetLastError();
		if (dwLastError == ERROR_FILE_NOT_FOUND ||
			dwLastError == ERROR_PATH_NOT_FOUND ||
			GetFileAttributes(pszFilePath) == INVALID_FILE_ATTRIBUTES)
		{
			bResult = TRUE; // ignore missing files
		}
	}
	return bResult;
}

/**
 * @param pszFileName - dump file name.
 * @return true if crash information has been written successfully.
 */
BOOL CSymEngine::WriteDump(PCTSTR pszFileName)
{
	_ASSERTE(FMiniDumpWriteDump != NULL);
	if (FMiniDumpWriteDump == NULL)
		return FALSE;
	HANDLE hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	HANDLE hProcess = GetCurrentProcess();
	DWORD dwProcessID = GetCurrentProcessId();
	BOOL bResult;
	if (m_pExceptionPointers != NULL)
	{
		MINIDUMP_EXCEPTION_INFORMATION ExInfo;
		ExInfo.ThreadId = GetCurrentThreadId();
		ExInfo.ExceptionPointers = m_pExceptionPointers;
		ExInfo.ClientPointers = TRUE;
		bResult = FMiniDumpWriteDump(hProcess, dwProcessID, hFile, g_eDumpType, &ExInfo, NULL, NULL);
	}
	else
		bResult = FMiniDumpWriteDump(hProcess, dwProcessID, hFile, g_eDumpType, NULL, NULL, NULL);
	CloseHandle(hFile);
	if (! bResult)
		DeleteFile(pszFileName);
	return bResult;
}

/**
 * @param pszFolderName - parent folder name.
 * @param pEnumProcess - pointer to the process enumerator;
 * pass NULL pointer if you want to skip process and module list.
 * @return true if crash information has been stored successfully.
 */
BOOL CSymEngine::WriteReportFiles(PCTSTR pszFolderName, CEnumProcess* pEnumProcess)
{
	PCTSTR pszLogExtension = GetLogFileExtension();
	if (pszLogExtension == NULL)
		return FALSE;
	TCHAR szLogFileName[MAX_PATH];
	_stprintf_s(szLogFileName, countof(szLogFileName), _T("errorlog.%s"), pszLogExtension);
	TCHAR szFullLogFileName[MAX_PATH];
	PathCombine(szFullLogFileName, pszFolderName, szLogFileName);
	if (! WriteLog(szFullLogFileName, pEnumProcess))
		return FALSE;

	if (g_eDumpType != MiniDumpNoDump)
	{
		TCHAR szFullDumpFileName[MAX_PATH];
		PathCombine(szFullDumpFileName, pszFolderName, _T("crashdump.dmp"));
		if (FMiniDumpWriteDump != NULL && ! WriteDump(szFullDumpFileName))
			return FALSE;
	}

	if (m_pScreenShot)
	{
		TCHAR szFullScreenShotFileName[MAX_PATH];
		PathCombine(szFullScreenShotFileName, pszFolderName, _T("screenshot.bmp"));
		if (! m_pScreenShot->WriteScreenShot(szFullScreenShotFileName))
			return FALSE;
	}

	return TRUE;
}

/**
 * @param pszReportFolder - path to folder containing report files.
 * @param pszArchiveFileName - zip file name.
 * @return true if crash information has been archived successfully.
 */
BOOL CSymEngine::ArchiveReportFiles(PCTSTR pszReportFolder, PCTSTR pszArchiveFileName)
{
	PCSTR pszArchiveFileNameA;
#ifdef _UNICODE
	CHAR szArchiveFileNameA[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, pszArchiveFileName, -1, szArchiveFileNameA, countof(szArchiveFileNameA), NULL, NULL);
	pszArchiveFileNameA = szArchiveFileNameA;
#else
	pszArchiveFileNameA = pszArchiveFileName;
#endif
	zipFile hZipFile = zipOpen(pszArchiveFileNameA, FALSE);
	if (! hZipFile)
		return FALSE;

	BOOL bResult = TRUE;
	TCHAR szFindFileTemplate[MAX_PATH];
	PathCombine(szFindFileTemplate, pszReportFolder, _T("*"));
	WIN32_FIND_DATA FindData;
	HANDLE hFindFile = FindFirstFile(szFindFileTemplate, &FindData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		BOOL bMore = TRUE;
		while (bMore)
		{
			if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				TCHAR szFilePath[MAX_PATH];
				PathCombine(szFilePath, pszReportFolder, FindData.cFileName);
				bResult = AddFileToArchive(hZipFile, szFilePath, FindData.cFileName);
				if (! bResult)
					break;
			}
			bMore = FindNextFile(hFindFile, &FindData);
		}
		FindClose(hFindFile);
	}

	if (bResult)
	{
		int nFileCount = g_arrLogLinks.GetCount();
		for (int nFilePos = 0; nFilePos < nFileCount; ++nFilePos)
		{
			CLogLink* pLogLink = g_arrLogLinks[nFilePos];
			_ASSERTE(pLogLink != NULL);
			PCTSTR pszFilePath = pLogLink->GetLogFileName();
			PCTSTR pszFileName = PathFindFileName(pszFilePath);
			_ASSERTE(pszFileName != NULL);
			bResult = AddFileToArchive(hZipFile, pszFilePath, pszFileName);
			if (! bResult)
				break;
		}
	}

	if (zipClose(hZipFile, NULL) != ZIP_OK)
		bResult = FALSE;
	if (! bResult)
		DeleteFile(pszArchiveFileName);
	return bResult;
}

/**
 * @param pszArchiveFileName - zip file name.
 * @param pEnumProcess - pointer to the process enumerator;
 * pass NULL pointer if you want to skip process and module list.
 * @return true if crash information has been archived successfully.
 */
BOOL CSymEngine::WriteReportArchive(PCTSTR pszArchiveFileName, CEnumProcess* pEnumProcess)
{
	TCHAR szTempPath[MAX_PATH];
	CreateTempFolder(szTempPath, countof(szTempPath));
	BOOL bResult = WriteReportFiles(szTempPath, pEnumProcess) &&
	               ArchiveReportFiles(szTempPath, pszArchiveFileName);
	DeleteFolder(szTempPath);
	return bResult;
}

/**
 * @param pszExtension - file extension.
 * @param pszFileName - buffer for resulting file name.
 * @param dwBufferSize - size of file name buffer.
 */
void CSymEngine::GetReportFileName(PCTSTR pszExtension, PTSTR pszFileName, DWORD dwBufferSize) const
{
	if (pszExtension == NULL)
		pszExtension = _T("");
	DWORD dwFileNameLen = GetCanonicalAppName(pszFileName, dwBufferSize, FALSE);
	if (dwFileNameLen > 0 && dwFileNameLen + 1 < dwBufferSize)
	{
		pszFileName[dwFileNameLen++] = _T('_');
		pszFileName[dwFileNameLen] = _T('\0');
	}
	_stprintf_s(pszFileName + dwFileNameLen, dwBufferSize - dwFileNameLen,
	            _T("%s_%02d%02d%02d-%02d%02d%02d.%s"),
				m_pExceptionPointers != NULL ? _T("error_report") : _T("snapshot"),
	            m_DateTime.wYear % 100, m_DateTime.wMonth, m_DateTime.wDay,
	            m_DateTime.wHour, m_DateTime.wMinute, m_DateTime.wSecond,
	            pszExtension);
}

/**
 * @param pszFileName - log file name.
 * @param pEnumProcess - pointer to the process enumerator;
 * pass NULL pointer if you want to skip process and module list.
 * @return true if crash information has been archived successfully.
 */
BOOL CSymEngine::WriteLog(PCTSTR pszFileName, CEnumProcess* pEnumProcess)
{
	CFileStream FileStream(1024);
	if (! FileStream.Open(pszFileName, CREATE_ALWAYS, GENERIC_WRITE))
		return FALSE;
	if (g_eReportFormat == BTRF_TEXT)
	{
		CUTF8EncStream EncStream(&FileStream);
		GetErrorLog(EncStream, pEnumProcess);
		return TRUE;
	}
	else if (g_eReportFormat == BTRF_XML)
	{
		CXmlWriter XmlWriter(&FileStream);
		GetErrorLog(XmlWriter, pEnumProcess);
		return TRUE;
	}
	else
	{
		_ASSERT(FALSE);
		return FALSE;
	}
}

/**
 * @param rEntry - stack entry information.
 * @return true if there is information about stack entry.
 */
BOOL CSymEngine::GetNextStackTraceEntry(CStackTraceEntry& rEntry)
{
	if (m_hDbgHelpDll == NULL)
		return FALSE;

	if (++m_dwFrameCount > MAX_FRAME_COUNT)
		return FALSE;

	BOOL bResult = FStackWalk64(IMAGE_FILE_MACHINE_I386,
	                            m_hSymProcess,
	                            m_swContext.m_hThread,
	                            &m_swContext.m_stFrame,
	                            &m_swContext.m_context,
	                            ReadProcessMemoryProc64,
	                            FSymFunctionTableAccess64,
	                            FSymGetModuleBase64,
	                            NULL);
	if (! bResult || ! m_swContext.m_stFrame.AddrFrame.Offset)
		return FALSE;

	HINSTANCE hModule = (HINSTANCE)FSymGetModuleBase64(m_hSymProcess, m_swContext.m_stFrame.AddrPC.Offset);
	if (hModule != NULL)
		GetModuleFileName(hModule, rEntry.m_szModule, countof(rEntry.m_szModule));
	DWORD64 dwExceptionAddress = m_swContext.m_stFrame.AddrPC.Offset;
	WORD wExceptionSegment; // wExceptionSegment = m_swContext.m_stFrame.AddrPC.Segment;
	__asm { mov word ptr [wExceptionSegment], cs }
	_stprintf_s(rEntry.m_szAddress, countof(rEntry.m_szAddress),
	            _T("%04lX:%08lX"), wExceptionSegment, dwExceptionAddress);

	BYTE arrSymBuffer[512];
	ZeroMemory(arrSymBuffer, sizeof(arrSymBuffer));
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)arrSymBuffer;
	pSymbol->SizeOfStruct = sizeof(*pSymbol);
	pSymbol->MaxNameLen = sizeof(arrSymBuffer) - sizeof(*pSymbol) + 1;
	DWORD64 dwDisplacement64;
	if (FSymFromAddr(m_hSymProcess, dwExceptionAddress, &dwDisplacement64, pSymbol))
	{
#ifdef _UNICODE
		MultiByteToWideChar(CP_ACP, 0, pSymbol->Name, -1, rEntry.m_szFunctionName, countof(rEntry.m_szFunctionName));
#else
		_tcscpy_s(rEntry.m_szFunctionName, countof(rEntry.m_szFunctionName), pSymbol->Name);
#endif
		if (dwDisplacement64)
		{
			_ui64tot_s(dwDisplacement64, rEntry.m_szFunctionOffset, countof(rEntry.m_szFunctionOffset), 10);
			_stprintf_s(rEntry.m_szFunctionInfo, countof(rEntry.m_szFunctionInfo), _T("%s()+%s byte(s)"), rEntry.m_szFunctionName, rEntry.m_szFunctionOffset);
		}
		else
		{
			*rEntry.m_szFunctionOffset = _T('\0');
			_stprintf_s(rEntry.m_szFunctionInfo, countof(rEntry.m_szFunctionInfo), _T("%s()"), rEntry.m_szFunctionName);
		}
	}
	else
	{
		*rEntry.m_szFunctionInfo = _T('\0');
		*rEntry.m_szFunctionName = _T('\0');
		*rEntry.m_szFunctionOffset = _T('\0');
	}

	DWORD dwDisplacement32;
	IMAGEHLP_LINE64 il;
	ZeroMemory(&il, sizeof(il));
	il.SizeOfStruct = sizeof(il);
	if (FSymGetLineFromAddr64(m_hSymProcess, dwExceptionAddress, &dwDisplacement32, &il))
	{
#ifdef _UNICODE
		MultiByteToWideChar(CP_ACP, 0, il.FileName, -1, rEntry.m_szSourceFile, countof(rEntry.m_szSourceFile));
#else
		_tcscpy_s(rEntry.m_szSourceFile, countof(rEntry.m_szSourceFile), il.FileName);
#endif
		_ultot_s(il.LineNumber, rEntry.m_szLineNumber, countof(rEntry.m_szLineNumber), 10);
		if (dwDisplacement32)
		{
			_ultot_s(dwDisplacement32, rEntry.m_szLineOffset, countof(rEntry.m_szLineOffset), 10);
			_stprintf_s(rEntry.m_szLineInfo, countof(rEntry.m_szLineInfo), _T("line %s+%s byte(s)"), rEntry.m_szLineNumber, rEntry.m_szLineOffset);
		}
		else
		{
			*rEntry.m_szLineOffset = _T('\0');
			_stprintf_s(rEntry.m_szLineInfo, countof(rEntry.m_szLineInfo), _T("line %s"), rEntry.m_szLineNumber);
		}
	}
	else
	{
		*rEntry.m_szSourceFile = _T('\0');
		*rEntry.m_szLineInfo = _T('\0');
		*rEntry.m_szLineNumber = _T('\0');
		*rEntry.m_szLineOffset = _T('\0');
	}

	return TRUE;
}

/**
* @param pszFileName - report file name.
* @param pEnumProcess - pointer to the process enumerator;
* pass NULL pointer if you want to skip process and module list.
* @return true if crash information has been archived successfully.
*/
BOOL CSymEngine::WriteReport(PCTSTR pszFileName, CEnumProcess* pEnumProcess)
{
	return (g_dwFlags & BTF_DETAILEDMODE ?
		WriteReportArchive(pszFileName, pEnumProcess) :
		WriteLog(pszFileName, pEnumProcess));
}

/**
 * @param rEncStream - UTF-8 encoder object.
 * @param hThread - handle of examined thread; pass NULL if you want to get exception stack trace.
 * @return true if thread context was successfully resolved.
 */
BOOL CSymEngine::GetFirstWin32StackTraceString(CUTF8EncStream& rEncStream, HANDLE hThread)
{
	if (! InitStackTrace(hThread))
		return FALSE;
	return GetNextWin32StackTraceString(rEncStream);
}

/**
* @param rEntry - stack entry information.
* @param hThread - handle of examined thread; pass NULL if you want to get exception stack trace.
* @return true if there is information about stack entry.
*/
BOOL CSymEngine::GetFirstStackTraceEntry(CStackTraceEntry& rEntry, HANDLE hThread)
{
	if (! InitStackTrace(hThread))
		return FALSE;
	return GetNextStackTraceEntry(rEntry);
}

/**
 * @return log file extension.
 */
PCTSTR CSymEngine::GetLogFileExtension(void)
{
	if (g_eReportFormat == BTRF_TEXT)
		return _T("log");
	else if (g_eReportFormat == BTRF_XML)
		return _T("xml");
	else
	{
		_ASSERT(FALSE);
		return NULL;
	}
}

/**
 * @return report file extension.
 */
PCTSTR CSymEngine::GetReportFileExtension(void)
{
	return (g_dwFlags & BTF_DETAILEDMODE ? _T("zip") : GetLogFileExtension());
}
