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

#include "StdAfx.h"
#include "NetThunks.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _MANAGED

#pragma managed /////////////////////////////////////////////////////////////////////////////////

using namespace System::IO;
using namespace System::Reflection;

namespace NetThunks
{

	void GetThreadInfo(gcroot<Thread^> gcThread, DWORD& dwThreadID, PWSTR pszThreadName, DWORD dwThreadNameSize)
	{
		try
		{
			Thread^ thread = gcThread;
#ifdef _DEBUG
			Debug::Assert(thread != nullptr);
#endif
			dwThreadID = thread->ManagedThreadId;
			if (dwThreadNameSize > 0)
			{
				String^ threadName = thread->Name;
				if (threadName != nullptr)
				{
					pin_ptr<const wchar_t> wstrThreadName(PtrToStringChars(threadName));
					wcscpy_s(pszThreadName, dwThreadNameSize, wstrThreadName);
				}
				else
					*pszThreadName = L'\0';
			}
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
			dwThreadID = MAXDWORD;
			if (dwThreadNameSize > 0)
				*pszThreadName = L'\0';
		}
	}

	void GetAppDomainInfo(DWORD& dwAppDomainID, PWSTR pszAppDomainName, DWORD dwAppDomainSize)
	{
		try
		{
			AppDomain^ appDomain = AppDomain::CurrentDomain;
#ifdef _DEBUG
			Debug::Assert(appDomain != nullptr);
#endif
			dwAppDomainID = appDomain->Id;
			if (dwAppDomainSize > 0)
			{
				String^ appDomainName = appDomain->FriendlyName;
				if (appDomainName != nullptr)
				{
					pin_ptr<const wchar_t> wstrAppDomainName(PtrToStringChars(appDomainName));
					wcscpy_s(pszAppDomainName, dwAppDomainSize, wstrAppDomainName);
				}
				else
					*pszAppDomainName = L'\0';
			}
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
			dwAppDomainID = MAXDWORD;
			if (dwAppDomainSize > 0)
				*pszAppDomainName = L'\0';
		}
	}

	static String^ GetMethodSignature(MethodBase^ method, bool addType)
	{
		MemberTypes memberType = method->MemberType;
		if ((unsigned)(memberType & (MemberTypes::Constructor | MemberTypes::Method)) == 0)
			return nullptr;
		StringBuilder^ signature = gcnew StringBuilder();
		if ((unsigned)(memberType & MemberTypes::Method) != 0)
		{
			signature->Append((safe_cast<MethodInfo^>(method))->ReturnType);
			signature->Append(L' ');
		}
		if (addType)
		{
			Type^ type = method->DeclaringType;
			if (type != nullptr)
			{
				signature->Append(type);
				signature->Append(L'.');
			}
		}
		signature->Append(method->Name);
		signature->Append(L'(');
		array<ParameterInfo^>^ parameters = method->GetParameters();
		int count = parameters->Length;
		for (int index = 0; index < count; ++index)
		{
			ParameterInfo^ parameter = parameters[index];
			if (index > 0)
				signature->Append(L", ");
			signature->Append(parameter->ParameterType);
			signature->Append(L' ');
			signature->Append(parameter->Name);
		}
		signature->Append(L')');
		return signature->ToString();
	}

	static void GetStackTraceEntry(StackFrame^ stackFrame, CNetStackTrace::CNetStackTraceEntry& rEntry)
	{
#ifdef _DEBUG
		Debug::Assert(stackFrame != nullptr);
#endif
		MethodBase^ method = stackFrame->GetMethod();
		Type^ type = method->DeclaringType;
		Assembly^ assembly = type->Assembly;
		AssemblyName^ assemblyName = assembly->GetName();
		String^ assemblyNameString = assemblyName != nullptr ? assemblyName->Name : nullptr;
		if (assemblyNameString != nullptr)
		{
			pin_ptr<const wchar_t> wstrAssembly(PtrToStringChars(assemblyNameString));
			wcscpy_s(rEntry.m_szAssembly, countof(rEntry.m_szAssembly), wstrAssembly);
		}
		else
			*rEntry.m_szAssembly = L'\0';
		int ilOffset = stackFrame->GetILOffset();
		if (ilOffset != StackFrame::OFFSET_UNKNOWN)
			_ui64tow_s(ilOffset, rEntry.m_szILOffset, countof(rEntry.m_szILOffset), 10);
		else
			*rEntry.m_szILOffset = L'\0';
		int nativeOffset = stackFrame->GetNativeOffset();
		if (nativeOffset != StackFrame::OFFSET_UNKNOWN)
			_ui64tow_s(nativeOffset, rEntry.m_szNativeOffset, countof(rEntry.m_szNativeOffset), 10);
		else
			*rEntry.m_szNativeOffset = L'\0';
		pin_ptr<const wchar_t> wstrType(PtrToStringChars(type->ToString()));
		wcscpy_s(rEntry.m_szType, countof(rEntry.m_szType), wstrType);
		String^ shortMethodSignature = GetMethodSignature(method, false);
		if (shortMethodSignature != nullptr)
		{
			pin_ptr<const wchar_t> wstrMethod(PtrToStringChars(shortMethodSignature));
			wcscpy_s(rEntry.m_szMethod, countof(rEntry.m_szMethod), wstrMethod);
		}
		else
			*rEntry.m_szMethod = L'\0';
		String^ fullMethodSignature = GetMethodSignature(method, true);
		if (fullMethodSignature != nullptr)
		{
			pin_ptr<const wchar_t> wstrMethodSignature(PtrToStringChars(fullMethodSignature));
			wcscpy_s(rEntry.m_szMethodSignature, countof(rEntry.m_szMethodSignature), wstrMethodSignature);
		}
		else
			*rEntry.m_szMethodSignature = L'\0';
		String^ fileName = stackFrame->GetFileName();
		if (fileName != nullptr)
		{
			pin_ptr<const wchar_t> wstrSourceFile(PtrToStringChars(fileName));
			wcscpy_s(rEntry.m_szSourceFile, countof(rEntry.m_szSourceFile), wstrSourceFile);
		}
		else
			*rEntry.m_szSourceFile = L'\0';
		int lineNumber = stackFrame->GetFileLineNumber();
		if (lineNumber != 0)
			_ultow_s(lineNumber, rEntry.m_szLineNumber, countof(rEntry.m_szLineNumber));
		else
			*rEntry.m_szLineNumber = L'\0';
		int columnNumber = stackFrame->GetFileColumnNumber();
		if (columnNumber != 0)
			_ultow_s(columnNumber, rEntry.m_szColumnNumber, countof(rEntry.m_szColumnNumber));
		else
			*rEntry.m_szColumnNumber = L'\0';
		if (lineNumber != 0)
		{
			if (columnNumber != 0)
				swprintf_s(rEntry.m_szLineInfo, countof(rEntry.m_szLineInfo), L"line %lu, column %lu", lineNumber, columnNumber);
			else
				swprintf_s(rEntry.m_szLineInfo, countof(rEntry.m_szLineInfo), L"line %lu", lineNumber);
		}
		else
			*rEntry.m_szLineInfo = L'\0';
	}

	gcroot<StackFrameEnumerator^> EnumStackFrames(void)
	{
		StackFrameEnumerator^ stackFrameEnumerator = nullptr;
		try
		{
#ifdef _DEBUG
			Debug::Assert(IsNetException());
#endif
			stackFrameEnumerator = gcnew StackFrameEnumerator(ExceptionHandler::Exception);
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
		}
		return gcroot<StackFrameEnumerator^>(stackFrameEnumerator);
	}

	gcroot<StackFrameEnumerator^> EnumStackFrames(gcroot<Thread^> thread)
	{
		StackFrameEnumerator^ stackFrameEnumerator = nullptr;
		try
		{
			stackFrameEnumerator = gcnew StackFrameEnumerator(thread);
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
		}
		return gcroot<StackFrameEnumerator^>(stackFrameEnumerator);
	}

	void InitStackTrace(gcroot<StackFrameEnumerator^> gcStackFrameEnumerator)
	{
		StackFrameEnumerator^ stackFrameEnumerator = gcStackFrameEnumerator;
#ifdef _DEBUG
		Debug::Assert(stackFrameEnumerator != nullptr);
#endif
		stackFrameEnumerator->InitStackTrace();
	}

	bool GetFirstStackTraceEntry(gcroot<StackFrameEnumerator^> gcStackFrameEnumerator, CNetStackTrace::CNetStackTraceEntry& rEntry)
	{
		InitStackTrace(gcStackFrameEnumerator);
		return GetNextStackTraceEntry(gcStackFrameEnumerator, rEntry);
	}

	bool GetNextStackTraceEntry(gcroot<StackFrameEnumerator^> gcStackFrameEnumerator, CNetStackTrace::CNetStackTraceEntry& rEntry)
	{
		try
		{
			StackFrameEnumerator^ stackFrameEnumerator = gcStackFrameEnumerator;
#ifdef _DEBUG
			Debug::Assert(stackFrameEnumerator != nullptr);
#endif
			StackFrame^ stackFrame = stackFrameEnumerator->GetNextStackTraceEntry();
			if (stackFrame == nullptr)
				return false;
			GetStackTraceEntry(stackFrame, rEntry);
			return true;
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
			return false;
		}
	}

	bool GetErrorInfo(gcroot<StackFrameEnumerator^> gcStackFrameEnumerator, CNetStackTrace::CNetErrorInfo& rErrorInfo)
	{
		try
		{
			if (! GetFirstStackTraceEntry(gcStackFrameEnumerator, rErrorInfo))
				return false;
			Exception^ exception = ExceptionHandler::Exception;
#ifdef _DEBUG
			Debug::Assert(exception != nullptr);
#endif
			Type^ type = exception->GetType();
			pin_ptr<const wchar_t> wstrException(PtrToStringChars(type->Name));
			wcscpy_s(rErrorInfo.m_szException, countof(rErrorInfo.m_szException), wstrException);
			String^ message = exception->Message;
			if (message != nullptr)
			{
				pin_ptr<const wchar_t> wstrMessage(PtrToStringChars(message));
				wcscpy_s(rErrorInfo.m_szMessage, countof(rErrorInfo.m_szMessage), wstrMessage);
			}
			else
				*rErrorInfo.m_szMessage = L'\0';
			Process^ process = Process::GetCurrentProcess();
#ifdef _DEBUG
			Debug::Assert(process != nullptr);
#endif
			_ultow_s(process->Id, rErrorInfo.m_szProcessID, countof(rErrorInfo.m_szProcessID));
			//String^ processName = process->ProcessName;
			String^ processName = Path::GetFileName(process->MainModule->ModuleName);
			if (processName != nullptr)
			{
				pin_ptr<const wchar_t> wstrProcessName(PtrToStringChars(processName));
				wcscpy_s(rErrorInfo.m_szProcessName, countof(rErrorInfo.m_szProcessName), wstrProcessName);
			}
			else
				*rErrorInfo.m_szProcessName = L'\0';
			AppDomain^ appDomain = AppDomain::CurrentDomain;
#ifdef _DEBUG
			Debug::Assert(appDomain != nullptr);
#endif
			String^ appDomainName = appDomain->FriendlyName;
			if (appDomainName != nullptr)
			{
				pin_ptr<const wchar_t> wstrAppDomainName(PtrToStringChars(appDomainName));
				wcscpy_s(rErrorInfo.m_szAppDomainName, countof(rErrorInfo.m_szAppDomainName), wstrAppDomainName);
			}
			else
				*rErrorInfo.m_szAppDomainName = L'\0';
			_ultow_s(appDomain->Id, rErrorInfo.m_szAppDomainID, countof(rErrorInfo.m_szAppDomainID));
			return true;
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
			return false;
		}
	}

	void GetNetVersion(PWSTR pszNetVersion, DWORD dwNetVersionSize)
	{
		try
		{
			if (dwNetVersionSize > 0)
			{
				Version^ version = Environment::Version;
				if (version != nullptr)
				{
					pin_ptr<const wchar_t> wstrNetVersion(PtrToStringChars(version->ToString()));
					wcscpy_s(pszNetVersion, dwNetVersionSize, wstrNetVersion);
				}
				else
					*pszNetVersion = L'\0';
			}
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
			if (dwNetVersionSize > 0)
				*pszNetVersion = L'\0';
		}
	}

	gcroot<AssemblyEnumerator^> EnumAssemblies(void)
	{
		AssemblyEnumerator^ assemblyEnumerator = nullptr;
		try
		{
			assemblyEnumerator = gcnew AssemblyEnumerator();
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
		}
		return gcroot<AssemblyEnumerator^>(assemblyEnumerator);
	}

	static void GetAssemblyInfo(Assembly^ assembly, CNetAssemblies::CAssemblyInfo& rAssemblyInfo)
	{
#ifdef _DEBUG
		Debug::Assert(assembly != nullptr);
#endif
		AssemblyName^ assemblyName = assembly->GetName();
		String^ assemblyNameString = assemblyName != nullptr ? assemblyName->Name : nullptr;
		if (assemblyNameString != nullptr)
		{
			pin_ptr<const wchar_t> wstrAssemblyName(PtrToStringChars(assemblyNameString));
			wcscpy_s(rAssemblyInfo.m_szName, countof(rAssemblyInfo.m_szName), wstrAssemblyName);
		}
		else
			*rAssemblyInfo.m_szName = L'\0';
		Version^ version = assemblyName != nullptr ? assemblyName->Version : nullptr;
		if (version != nullptr)
		{
			pin_ptr<const wchar_t> wstrAssemblyVersion(PtrToStringChars(version->ToString()));
			wcscpy_s(rAssemblyInfo.m_szVersion, countof(rAssemblyInfo.m_szVersion), wstrAssemblyVersion);
		}
		else
			*rAssemblyInfo.m_szVersion = L'\0';
		FileVersionInfo^ fileVersion = FileVersionInfo::GetVersionInfo(assembly->Location);
		String^ fileVersionString = fileVersion != nullptr ? fileVersion->FileVersion : nullptr;
		if (fileVersionString != nullptr)
		{
			pin_ptr<const wchar_t> wstrFileVersion(PtrToStringChars(fileVersionString));
			wcscpy_s(rAssemblyInfo.m_szFileVersion, countof(rAssemblyInfo.m_szFileVersion), wstrFileVersion);
		}
		else
			*rAssemblyInfo.m_szFileVersion = L'\0';
		String^ codeBase = assemblyName->CodeBase;
		if (codeBase != nullptr)
		{
			pin_ptr<const wchar_t> wstrCodeBase(PtrToStringChars(codeBase));
			wcscpy_s(rAssemblyInfo.m_szCodeBase, countof(rAssemblyInfo.m_szCodeBase), wstrCodeBase);
		}
		else
			*rAssemblyInfo.m_szCodeBase = L'\0';
	}

	void InitAssemblies(gcroot<AssemblyEnumerator^> gcAssemblyEnumerator)
	{
		AssemblyEnumerator^ assemblyEnumerator = gcAssemblyEnumerator;
#ifdef _DEBUG
		Debug::Assert(assemblyEnumerator != nullptr);
#endif
		assemblyEnumerator->InitAssemblies();
	}

	bool GetFirstAssembly(gcroot<AssemblyEnumerator^> gcAssemblyEnumerator, CNetAssemblies::CAssemblyInfo& rAssemblyInfo)
	{
		InitAssemblies(gcAssemblyEnumerator);
		return GetNextAssembly(gcAssemblyEnumerator, rAssemblyInfo);
	}

	bool GetNextAssembly(gcroot<AssemblyEnumerator^> gcAssemblyEnumerator, CNetAssemblies::CAssemblyInfo& rAssemblyInfo)
	{
		try
		{
			AssemblyEnumerator^ assemblyEnumerator = gcAssemblyEnumerator;
#ifdef _DEBUG
			Debug::Assert(assemblyEnumerator != nullptr);
#endif
			Assembly^ assembly = assemblyEnumerator->GetNextAssembly();
			if (assembly == nullptr)
				return false;
			GetAssemblyInfo(assembly, rAssemblyInfo);
			return true;
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
			return false;
		}
	}

	bool ReadVersionInfo(void)
	{
		try
		{
			ExceptionHandler::ReadVersionInfo();
			return true;
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
			return false;
		}
	}

	void FireBeforeUnhandledExceptionEvent(void)
	{
		try
		{
			ExceptionHandler::FireBeforeUnhandledExceptionEvent();
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
		}
	}

	void FireAfterUnhandledExceptionEvent(void)
	{
		try
		{
			ExceptionHandler::FireAfterUnhandledExceptionEvent();
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
		}
	}

	void FlushTraceListeners(void)
	{
		try
		{
			Trace::Close();
			Trace::Listeners->Clear();
			Debug::Close();
			Debug::Listeners->Clear();
			Debug::Listeners->Add(gcnew DefaultTraceListener());
		}
		catch (Exception^ exception)
		{
			Debug::WriteLine(exception);
		}
	}

	gcroot<Thread^> GetCurrentThread(void)
	{
		return gcroot<Thread^>(Thread::CurrentThread);
	}

}

#pragma unmanaged ///////////////////////////////////////////////////////////////////////////////

CNetStackTrace::CNetStackTraceEntry::CNetStackTraceEntry(void)
{
	*m_szAssembly = L'\0';
	*m_szILOffset = L'\0';
	*m_szNativeOffset = L'\0';
	*m_szType = L'\0';
	*m_szMethod = L'\0';
	*m_szMethodSignature = L'\0';
	*m_szSourceFile = L'\0';
	*m_szLineNumber = L'\0';
	*m_szColumnNumber = L'\0';
	*m_szLineInfo = L'\0';
}

CNetStackTrace::CNetErrorInfo::CNetErrorInfo(void)
{
	*m_szException = L'\0';
	*m_szMessage = L'\0';
	*m_szAppDomainName = L'\0';
	*m_szAppDomainID = L'\0';
	*m_szProcessName = L'\0';
	*m_szProcessID = L'\0';
}

void CNetStackTrace::GetStackTraceString(const CNetStackTraceEntry& rEntry, CUTF8EncStream& rEncStream)
{
	if (*rEntry.m_szAssembly)
	{
		rEncStream.WriteByte('\"');
		rEncStream.WriteUTF8Bin(rEntry.m_szAssembly);
		rEncStream.WriteByte('\"');
	}

	if (*rEntry.m_szILOffset || *rEntry.m_szNativeOffset)
	{
		rEncStream.WriteAscii(" at");
		if (*rEntry.m_szILOffset)
		{
			rEncStream.WriteAscii(" IL:");
			rEncStream.WriteUTF8Bin(rEntry.m_szILOffset);
		}

		if (*rEntry.m_szNativeOffset)
		{
			if (*rEntry.m_szILOffset)
				rEncStream.WriteByte(',');
			rEncStream.WriteAscii(" NATIVE:");
			rEncStream.WriteUTF8Bin(rEntry.m_szNativeOffset);
		}
	}

	if (*rEntry.m_szMethodSignature)
	{
		rEncStream.WriteAscii(", ");
		rEncStream.WriteUTF8Bin(rEntry.m_szMethodSignature);
	}

	if (*rEntry.m_szSourceFile)
	{
		rEncStream.WriteAscii(" in \"");
		rEncStream.WriteUTF8Bin(rEntry.m_szSourceFile);
		rEncStream.WriteByte('\"');
	}

	if (*rEntry.m_szLineInfo)
	{
		rEncStream.WriteAscii(", ");
		rEncStream.WriteUTF8Bin(rEntry.m_szLineInfo);
	}
}

bool CNetStackTrace::GetFirstStackTraceString(CUTF8EncStream& rEncStream)
{
	InitStackTrace();
	return GetNextStackTraceString(rEncStream);
}

bool CNetStackTrace::GetNextStackTraceString(CUTF8EncStream& rEncStream)
{
	CNetStackTraceEntry Entry;
	if (! GetNextStackTraceEntry(Entry))
		return false;
	GetStackTraceString(Entry, rEncStream);
	return true;
}

void CNetStackTrace::GetErrorString(CStrStream& rStream)
{
	CNetErrorInfo ErrorInfo;
	if (! GetErrorInfo(ErrorInfo))
	{
		rStream << L"Error information is not available";
		return;
	}

	if (*ErrorInfo.m_szAppDomainName && _wcsicmp(ErrorInfo.m_szAppDomainName, ErrorInfo.m_szProcessName) != 0)
	{
		rStream << L"domain ";
		rStream << ErrorInfo.m_szAppDomainName;
		rStream << L" of ";
	}

	rStream << ErrorInfo.m_szProcessName;
	rStream << L" caused ";
	rStream << ErrorInfo.m_szException;

	const WCHAR wszStandardExceptionPrefix[] = L"Exception of type ";
	const DWORD dwStandardExceptionPrefixLength = countof(wszStandardExceptionPrefix) - 1;
	if (*ErrorInfo.m_szMessage && wcsncmp(ErrorInfo.m_szMessage, wszStandardExceptionPrefix, dwStandardExceptionPrefixLength) != 0)
	{
		rStream << L" (";
		rStream << ErrorInfo.m_szMessage;
		rStream << L')';
	}

	if (*ErrorInfo.m_szAssembly)
	{
		rStream << L" in assembly \"";
		rStream << ErrorInfo.m_szAssembly;
		rStream << L'\"';
	}

	if (*ErrorInfo.m_szILOffset || *ErrorInfo.m_szNativeOffset)
	{
		rStream << L" at";
		if (*ErrorInfo.m_szILOffset)
		{
			rStream << L" IL:";
			rStream << ErrorInfo.m_szILOffset;
		}

		if (*ErrorInfo.m_szNativeOffset)
		{
			if (*ErrorInfo.m_szILOffset)
				rStream << L',';
			rStream << L" NATIVE:";
			rStream << ErrorInfo.m_szNativeOffset;
		}
	}

	if (*ErrorInfo.m_szMethodSignature)
	{
		rStream << _T(", ");
		rStream << ErrorInfo.m_szMethodSignature;
	}

	if (*ErrorInfo.m_szSourceFile)
	{
		rStream << L" in \"";
		rStream << ErrorInfo.m_szSourceFile;
		rStream << L'\"';

		if (*ErrorInfo.m_szLineInfo)
		{
			rStream << L", ";
			rStream << ErrorInfo.m_szLineInfo;
		}
	}
}

void CNetStackTrace::GetErrorString(CUTF8EncStream& rEncStream)
{
	CStrStream Stream(1024);
	GetErrorString(Stream);
	rEncStream.WriteUTF8Bin(Stream);
}

CNetAssemblies::CAssemblyInfo::CAssemblyInfo(void)
{
	*m_szName = L'\0';
	*m_szVersion = L'\0';
	*m_szFileVersion = L'\0';
	*m_szCodeBase = L'\0';
}

#endif // _MANAGED
