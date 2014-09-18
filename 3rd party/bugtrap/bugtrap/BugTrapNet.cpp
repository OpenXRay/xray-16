/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: .NET interface to BugTrap.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "BugTrapNet.h"

#ifdef _MANAGED

#pragma managed

using namespace System::IO;
using namespace System::Text;

namespace IntelleSoft
{
	namespace BugTrap
	{
		Object^ LogFilesEnumerator::Current::get(void)
		{
			if (this->index < 0 || this->index >= LogFiles::Count)
				throw gcnew InvalidOperationException();
			return LogFiles::GetEntry(this->index);
		}

		bool LogFilesEnumerator::MoveNext(void)
		{
			int count = LogFiles::Count;
			if (this->index >= count)
				return false;
			return (++this->index < count);
		}

		Object^ LogFiles::GetLogFileEntry(INT_PTR index, bool getByIndex)
		{
			BUGTRAP_LOGTYPE type;
			DWORD size, result;
			result = BT_GetLogFileEntry(index, getByIndex, &type, NULL, NULL);
			if (result != ERROR_SUCCESS)
				throw gcnew Win32Exception(result);
			switch (type)
			{
			case BTLT_LOGFILE:
				{
					BUGTRAP_LOGFILEENTRY entry;
					size = sizeof(entry);
					result = BT_GetLogFileEntry(index, getByIndex, &type, &size, &entry);
					if (result != ERROR_SUCCESS)
						throw gcnew Win32Exception(result);
					return gcnew LogFileEntry(gcnew String(entry.szLogFileName));
				}
			case BTLT_REGEXPORT:
				{
					BUGTRAP_REGEXPORTENTRY entry;
					size = sizeof(entry);
					result = BT_GetLogFileEntry(index, getByIndex, &type, &size, &entry);
					if (result != ERROR_SUCCESS)
						throw gcnew Win32Exception(result);
					return gcnew RegExportEntry(gcnew String(entry.szLogFileName), gcnew String(entry.szRegKey));
				}
			default:
				throw gcnew InvalidOperationException();
			}
		}

		void ExceptionHandler::HandleException(System::Exception^ exception, Object^ sender, UnhandledExceptionEventArgs^ args)
		{
			Monitor::Enter(syncObj);
			try
			{
				Exception = exception;
				Sender = sender;
				Arguments = args;
				BT_CallNetFilter();
				Exception = nullptr;
				Sender = nullptr;
				Arguments = nullptr;
			}
			finally
			{
				Monitor::Exit(syncObj);
			}
		}

#ifdef _DEBUG
		void ExceptionHandler::HandleException(System::Exception^ exception)
		{
			UnhandledExceptionEventArgs^ args = gcnew UnhandledExceptionEventArgs(exception, ExceptionType::DomainException);
			HandleException(exception, nullptr, args);
		}
#endif

		void ExceptionHandler::ReadVersionInfo(AssemblyName^ assemblyName)
		{
			ExceptionHandler::AppName = assemblyName->Name;
			Version^ version = assemblyName->Version;
			ExceptionHandler::AppVersion = version != nullptr ? version->ToString() : nullptr;
		}

		void ExceptionHandler::InstallHandler(void)
		{
			BT_InstallSehFilter();
			AppDomain::CurrentDomain->UnhandledException += gcnew UnhandledExceptionEventHandler(OnUnhandledException);
			Application::ThreadException += gcnew ThreadExceptionEventHandler(OnThreadException);
		}

		void ExceptionHandler::UninstallHandler(void)
		{
			AppDomain::CurrentDomain->UnhandledException -= gcnew UnhandledExceptionEventHandler(OnUnhandledException);
			Application::ThreadException -= gcnew ThreadExceptionEventHandler(OnThreadException);
			BT_UninstallSehFilter();
		}

		void LogFile::PrvOpen(String^ fileName, LogFormatType logFormat)
		{
			pin_ptr<const wchar_t> wstrFileName(PtrToStringChars(fileName));
			this->handle = (IntPtr)BT_OpenLogFile(wstrFileName, (BUGTRAP_LOGFORMAT)logFormat);
			if (this->handle == IntPtr::Zero)
				throw gcnew IOException();
		}

		void LogFile::Close(void)
		{
			if (this->handle != IntPtr::Zero)
			{
				BT_CloseLogFile((INT_PTR)this->handle);
				Detach();
			}
		}

		void LogFile::Attach(IntPtr handle)
		{
			if (this->handle != IntPtr::Zero || handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			this->handle = handle;
		}

		void LogFile::Flush(void)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			BT_FlushLogFile((INT_PTR)this->handle);
		}

		String^ LogFile::FileName::get(void)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			return gcnew String(BT_GetLogFileName((INT_PTR)this->handle));
		}

		int LogFile::LogSizeInEntries::get(void)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			return BT_GetLogSizeInEntries((INT_PTR)this->handle);
		}

		void LogFile::LogSizeInEntries::set(int value)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			BT_SetLogSizeInEntries((INT_PTR)this->handle, value);
		}

		int LogFile::LogSizeInBytes::get(void)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			return BT_GetLogSizeInBytes((INT_PTR)this->handle);
		}

		void LogFile::LogSizeInBytes::set(int value)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			BT_SetLogSizeInBytes((INT_PTR)this->handle, value);
		}

		LogFlagsType LogFile::LogFlags::get(void)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			return (LogFlagsType)BT_GetLogFlags((INT_PTR)this->handle);
		}

		void LogFile::LogFlags::set(LogFlagsType value)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			return BT_SetLogFlags((INT_PTR)this->handle, (DWORD)value);
		}

		LogLevelType LogFile::LogLevel::get(void)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			return (LogLevelType)BT_GetLogLevel((INT_PTR)this->handle);
		}

		void LogFile::LogLevel::set(LogLevelType value)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			BT_SetLogLevel((INT_PTR)this->handle, (BUGTRAP_LOGLEVEL)value);
		}

		LogEchoType LogFile::LogEchoMode::get(void)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			return (LogEchoType)BT_GetLogEchoMode((INT_PTR)this->handle);
		}

		void LogFile::LogEchoMode::set(LogEchoType value)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			BT_SetLogEchoMode((INT_PTR)this->handle, (DWORD)value);
		}

		void LogFile::Clear(void)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			BT_ClearLog((INT_PTR)this->handle);
		}

		void LogFile::Insert(LogLevelType logLevel, String^ entry)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			pin_ptr<const wchar_t> wstrEntry(PtrToStringChars(entry));
			BT_InsLogEntry((INT_PTR)this->handle, (BUGTRAP_LOGLEVEL)logLevel, wstrEntry);
		}

		void LogFile::Insert(LogLevelType logLevel, String^ format, ... array<Object^>^ args)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			StringBuilder^ entry = gcnew StringBuilder();
			entry->AppendFormat(format, args);
			pin_ptr<const wchar_t> wstrEntry(PtrToStringChars(entry->ToString()));
			BT_InsLogEntry((INT_PTR)this->handle, (BUGTRAP_LOGLEVEL)logLevel, wstrEntry);
		}

		void LogFile::Append(LogLevelType logLevel, String^ entry)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			pin_ptr<const wchar_t> wstrEntry(PtrToStringChars(entry));
			BT_AppLogEntry((INT_PTR)this->handle, (BUGTRAP_LOGLEVEL)logLevel, wstrEntry);
		}

		void LogFile::Append(LogLevelType logLevel, String^ format, ... array<Object^>^ args)
		{
			if (this->handle == IntPtr::Zero)
				throw gcnew InvalidOperationException();
			StringBuilder^ entry = gcnew StringBuilder();
			entry->AppendFormat(format, args);
			pin_ptr<const wchar_t> wstrEntry(PtrToStringChars(entry->ToString()));
			BT_AppLogEntry((INT_PTR)this->handle, (BUGTRAP_LOGLEVEL)logLevel, wstrEntry);
		}
	}
}

#pragma unmanaged

#endif // _MANAGED
