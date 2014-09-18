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

#pragma once

#ifdef _MANAGED

#include "BugTrap.h"

#pragma managed

using namespace System;
using namespace System::Threading;
using namespace System::Windows::Forms;
using namespace System::Reflection;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Diagnostics;

#include <vcclr.h>

namespace IntelleSoft
{
	namespace BugTrap
	{
		public enum class ExceptionType
		{
			NativeException,
			DomainException,
			ThreadException
		};

		public enum class ActivityType
		{
			ShowUI     = BTA_SHOWUI,
			SaveReport = BTA_SAVEREPORT,
			MailReport = BTA_MAILREPORT,
			SendReport = BTA_SENDREPORT
		};

		[Flags]
		public enum class FlagsType
		{
			None           = BTF_NONE,
			DetailedMode   = BTF_DETAILEDMODE,
			EditMail       = BTF_EDITMAIL,
			AttachReport   = BTF_ATTACHREPORT,
			ListProcesses  = BTF_LISTPROCESSES,
			ShowAdvancedUI = BTF_SHOWADVANCEDUI,
			ScreenCapture  = BTF_SCREENCAPTURE,
			NativeInfo     = BTF_NATIVEINFO
		};

		public enum class LogLevelType
		{
			None    = BTLL_NONE,
			Error   = BTLL_ERROR,
			Warning = BTLL_WARNING,
			Info    = BTLL_INFO,
			All     = BTLL_ALL
		};

		[Flags]
		public enum class LogEchoType
		{
			None   = BTLE_NONE,
			StdOut = BTLE_STDOUT,
			StdErr = BTLE_STDERR,
			DbgOut = BTLE_DBGOUT
		};

		[Flags]
		public enum class LogFlagsType
		{
			None          = BTLF_NONE,
			ShowLogLevel  = BTLF_SHOWLOGLEVEL,
			ShowTimeStamp = BTLF_SHOWTIMESTAMP
		};

		public enum class ReportFormatType
		{
			Xml  = BTRF_XML,
			Text = BTRF_TEXT
		};

		public enum class LogFormatType
		{
			Xml  = BTLF_XML,
			Text = BTLF_TEXT
		};

		public enum class DialogMessageType
		{
			Intro1 = BTDM_INTRO1,
			Intro2 = BTDM_INTRO2
		};

		public enum class LogType
		{
			LogFile   = BTLT_LOGFILE,
			RegExport = BTLT_REGEXPORT
		};

		[Flags]
		public enum class MinidumpType
		{
			Normal                         = MiniDumpNormal,
			WithDataSegs                   = MiniDumpWithDataSegs,
			WithFullMemory                 = MiniDumpWithFullMemory,
			WithHandleData                 = MiniDumpWithHandleData,
			FilterMemory                   = MiniDumpFilterMemory,
			ScanMemory                     = MiniDumpScanMemory,
			WithUnloadedModules            = MiniDumpWithUnloadedModules,
			WithIndirectlyReferencedMemory = MiniDumpWithIndirectlyReferencedMemory,
			FilterModulePaths              = MiniDumpFilterModulePaths,
			WithProcessThreadData          = MiniDumpWithProcessThreadData,
			WithPrivateReadWriteMemory     = MiniDumpWithPrivateReadWriteMemory,
			WithoutOptionalData            = MiniDumpWithoutOptionalData,
			NoDump                         = MiniDumpNoDump
		};

		public ref class BaseLogFileEntry
		{
		public:
			inline BaseLogFileEntry() { }
			inline BaseLogFileEntry(String^ fileName) : LogFileName(fileName) { }
			virtual String^ ToString(void) override { return LogFileName; }
			String^ LogFileName;
		};

		public ref class LogFileEntry : public BaseLogFileEntry
		{
		public:
			inline LogFileEntry() { }
			inline LogFileEntry(String^ fileName) : BaseLogFileEntry(fileName) { }
		};

		public ref class RegExportEntry : public BaseLogFileEntry
		{
		public:
			inline RegExportEntry() { }
			inline RegExportEntry(String^ fileName, String^ regKey) : BaseLogFileEntry(fileName), RegKey(regKey) { }
			String^ RegKey;
		};

		private ref class LogFilesEnumerator sealed : public IEnumerator
		{
		public:
			LogFilesEnumerator(void);
			virtual bool MoveNext(void) sealed;
			virtual void Reset(void) sealed;
			virtual property Object^ Current
			{
				Object^ get() sealed;
			}

		private:
			int index;
		};

		inline LogFilesEnumerator::LogFilesEnumerator(void)
		{
			Reset();
		}

		inline void LogFilesEnumerator::Reset(void)
		{
			this->index = -1;
		}

		private ref class LogFilesEnumerable sealed : public IEnumerable
		{
		public:
			virtual IEnumerator^ GetEnumerator(void) sealed;
		};

		inline IEnumerator^ LogFilesEnumerable::GetEnumerator(void)
		{
			return gcnew LogFilesEnumerator();
		}

		public ref class LogFiles
		{
		public:
			static void Add(LogFileEntry^ entry);
			static void Add(RegExportEntry^ entry);
			static void Delete(String^ fileName);
			static void Clear(void);
			static property int Count
			{
				int get(void);
			}
			static Object^ GetEntry(int index);
			static Object^ GetEntry(String^ fileName);
			static property IEnumerable^ Entries
			{
				IEnumerable^ get(void);
			}

		private:
			static Object^ GetLogFileEntry(INT_PTR index, bool getByIndex);
		};

		inline void LogFiles::Add(LogFileEntry^ entry)
		{
			pin_ptr<const wchar_t> wstrFile(PtrToStringChars(entry->LogFileName));
			BT_AddLogFile(wstrFile);
		}

		inline void LogFiles::Add(RegExportEntry^ entry)
		{
			pin_ptr<const wchar_t> wstrFile(PtrToStringChars(entry->LogFileName));
			pin_ptr<const wchar_t> wstrKey(PtrToStringChars(entry->RegKey));
			BT_AddRegFile(wstrFile, wstrKey);
		}

		inline void LogFiles::Delete(String^ fileName)
		{
			pin_ptr<const wchar_t> wstrFileName(PtrToStringChars(fileName));
			BT_DeleteLogFile(wstrFileName);
		}

		inline void LogFiles::Clear(void)
		{
			BT_ClearLogFiles();
		}

		inline int LogFiles::Count::get(void)
		{
			return BT_GetLogFilesCount();
		}

		inline Object^ LogFiles::GetEntry(int index)
		{
			return GetLogFileEntry(index, true);
		}

		inline Object^ LogFiles::GetEntry(String^ fileName)
		{
			pin_ptr<const wchar_t> wstrFileName(PtrToStringChars(fileName));
			return GetLogFileEntry((INT_PTR)wstrFileName, false);
		}

		inline IEnumerable^ LogFiles::Entries::get(void)
		{
			return gcnew LogFilesEnumerable();
		}

		public ref class UnhandledExceptionEventArgs : public System::UnhandledExceptionEventArgs
		{
		public:
			UnhandledExceptionEventArgs(Exception^ exception, BugTrap::ExceptionType type);
			UnhandledExceptionEventArgs(System::UnhandledExceptionEventArgs^ args);
			UnhandledExceptionEventArgs(System::Threading::ThreadExceptionEventArgs^ args);
			property BugTrap::ExceptionType ExceptionType
			{
				BugTrap::ExceptionType get(void);
			}

		private:
			BugTrap::ExceptionType type;
		};

		inline UnhandledExceptionEventArgs::UnhandledExceptionEventArgs(Exception^ exception, BugTrap::ExceptionType type)
			: System::UnhandledExceptionEventArgs(exception, true)
		{
			this->type = type;
		}

		inline UnhandledExceptionEventArgs::UnhandledExceptionEventArgs(System::UnhandledExceptionEventArgs^ args) : System::UnhandledExceptionEventArgs(args->ExceptionObject, true)
		{
			this->type = BugTrap::ExceptionType::DomainException;
		}

		inline UnhandledExceptionEventArgs::UnhandledExceptionEventArgs(System::Threading::ThreadExceptionEventArgs^ args) : System::UnhandledExceptionEventArgs(args->Exception, true)
		{
			this->type = BugTrap::ExceptionType::ThreadException;
		}

		inline BugTrap::ExceptionType UnhandledExceptionEventArgs::ExceptionType::get(void)
		{
			return this->type;
		}

		public ref class LogFile
		{
		public:
			LogFile(void);
			LogFile(String^ fileName, LogFormatType logFormat);
			LogFile(IntPtr handle);
			~LogFile(); // Dispose()
			!LogFile(); // Finalize()
			void Open(String^ fileName, LogFormatType logFormat);
			void Attach(IntPtr handle);
			void Detach(void);
			void Close(void);
			property LogLevelType DefaultLogLevel
			{
				LogLevelType get(void);
				void set(LogLevelType value);
			}
			void Flush(void);
			property IntPtr Handle
			{
				IntPtr get(void);
			}
			property String^ FileName
			{
				String^ get(void);
			}
			property int LogSizeInEntries
			{
				int get(void);
				void set(int value);
			}
			property int LogSizeInBytes
			{
				int get(void);
				void set(int value);
			}
			property LogFlagsType LogFlags
			{
				LogFlagsType get(void);
				void set(LogFlagsType value);
			}
			property LogLevelType LogLevel
			{
				LogLevelType get(void);
				void set(LogLevelType value);
			}
			property LogEchoType LogEchoMode
			{
				LogEchoType get(void);
				void set(LogEchoType value);
			}
			void Clear(void);
			void Insert(String^ entry);
			void Insert(String^ format, ... array<Object^>^ args);
			void Insert(LogLevelType logLevel, String^ entry);
			void Insert(LogLevelType logLevel, String^ format, ... array<Object^>^ args);
			void Append(String^ entry);
			void Append(String^ format, ... array<Object^>^ args);
			void Append(LogLevelType logLevel, String^ entry);
			void Append(LogLevelType logLevel, String^ format, ... array<Object^>^ args);
			virtual String^ ToString(void) override { return FileName; }

		private:
			void PrvOpen(String^ fileName, LogFormatType logFormat);

			IntPtr handle;
			LogLevelType defaultLogLevel;
		};

		public delegate void UnhandledExceptionDelegate(Object^ sender, UnhandledExceptionEventArgs^ args);

		public ref class ExceptionHandler
		{
		private:
			static ExceptionHandler(void);
			static void OnUnhandledException(Object^ sender, System::UnhandledExceptionEventArgs^ args);
			static void OnThreadException(Object^ sender, System::Threading::ThreadExceptionEventArgs^ args);

			initonly static Object^ syncObj = gcnew Object();
			static System::Exception^ exception;
			static UnhandledExceptionEventArgs^ args;
			static Object^ sender;

			static event UnhandledExceptionDelegate^ beforeUnhandledExceptionEvent;
			static event UnhandledExceptionDelegate^ afterUnhandledExceptionEvent;
			static void HandleException(System::Exception^ exception, Object^ sender, UnhandledExceptionEventArgs^ args);

#ifdef _DEBUG
		public:
			static void HandleException(System::Exception^ exception);
#endif

		internal:
			static property System::Exception^ Exception
			{
			internal:
				System::Exception^ get(void);
			private:
				void set(System::Exception^ value);
			}

			static property Object^ Sender
			{
			internal:
				Object^ get(void);
			private:
				void set(Object^ value);
			}

			static property UnhandledExceptionEventArgs^ Arguments
			{
			internal:
				UnhandledExceptionEventArgs^ get(void);
			private:
				void set(UnhandledExceptionEventArgs^ value);
			}

			static void FireBeforeUnhandledExceptionEvent(void);
			static void FireAfterUnhandledExceptionEvent(void);

		public:
			static const int HttpPort = BUGTRAP_HTTP_PORT;

			static event UnhandledExceptionDelegate^ BeforeUnhandledException
			{
				void add(UnhandledExceptionDelegate^ value);
				void remove(UnhandledExceptionDelegate^ value);
			}

			static event UnhandledExceptionDelegate^ AfterUnhandledException
			{
				void add(UnhandledExceptionDelegate^ value);
				void remove(UnhandledExceptionDelegate^ value);
			}

			static property String^ AppName
			{
				String^ get(void);
				void set(String^ value);
			}

			static property String^ AppVersion
			{
				String^ get(void);
				void set(String^ value);
			}

			static void ReadVersionInfo(AssemblyName^ assemblyName);
			static void ReadVersionInfo(Assembly^ assembly);
			static void ReadVersionInfo(void);
			String^ GetDialogMessage(DialogMessageType dlgMsg);
			void SetDialogMessage(DialogMessageType dlgMsg, String^ value);

			static property String^ SupportURL
			{
				String^ get(void);
				void set(String^ value);
			}

			static property String^ SupportEMail
			{
				String^ get(void);
				void set(String^ value);
			}

			static property String^ SupportHost
			{
				String^ get(void);
				void set(String^ value);
			}

			static property short SupportPort
			{
				short get(void);
				void set(short value);
			}

			static property String^ NotificationEMail
			{
				String^ get(void);
				void set(String^ value);
			}

			static property FlagsType Flags
			{
				FlagsType get(void);
				void set(FlagsType value);
			}

			static property MinidumpType DumpType
			{
				MinidumpType get(void);
				void set(MinidumpType value);
			}

			static property ReportFormatType ReportFormat
			{
				ReportFormatType get(void);
				void set(ReportFormatType value);
			}

			static property String^ UserMessage
			{
				String^ get(void);
				void set(String^ value);
			}

			static property ActivityType Activity
			{
				ActivityType get(void);
				void set(ActivityType value);
			}

			static property String^ ReportFilePath
			{
				String^ get(void);
				void set(String^ value);
			}

			static property String^ MailProfile
			{
				String^ get(void);
			}

			static void SetMailProfile(String^ profile, String^ password);
			static void ExportRegKey(String^ fileName, String^ key);
			static void MakeSnapshot(String^ fileName);
			static void InstallHandler(void);
			static void UninstallHandler(void);
		};

		static inline ExceptionHandler::ExceptionHandler(void)
		{
			InstallHandler();
		}

		inline void ExceptionHandler::OnUnhandledException(Object^ sender, System::UnhandledExceptionEventArgs^ args)
		{
			HandleException(dynamic_cast<System::Exception^>(args->ExceptionObject), sender, gcnew UnhandledExceptionEventArgs(args));
		}

		inline void ExceptionHandler::OnThreadException(Object^ sender, System::Threading::ThreadExceptionEventArgs^ args)
		{
			HandleException(args->Exception, sender, gcnew UnhandledExceptionEventArgs(args));
		}

		inline System::Exception^ ExceptionHandler::Exception::get(void)
		{
			return exception;
		}

		inline void ExceptionHandler::Exception::set(System::Exception^ value)
		{
			Debug::Assert(value == nullptr || exception == nullptr);
			exception = value;
		}

		inline Object^ ExceptionHandler::Sender::get(void)
		{
			return sender;
		}

		inline void ExceptionHandler::Sender::set(Object^ value)
		{
			Debug::Assert(value == nullptr || sender == nullptr);
			sender = value;
		}

		inline UnhandledExceptionEventArgs^ ExceptionHandler::Arguments::get(void)
		{
			return args;
		}

		inline void ExceptionHandler::Arguments::set(UnhandledExceptionEventArgs^ value)
		{
			Debug::Assert(value == nullptr || args == nullptr);
			args = value;
		}

		inline void ExceptionHandler::FireBeforeUnhandledExceptionEvent(void)
		{
			beforeUnhandledExceptionEvent(Sender, Arguments);
		}

		inline void ExceptionHandler::FireAfterUnhandledExceptionEvent(void)
		{
			afterUnhandledExceptionEvent(Sender, Arguments);
		}

		inline void ExceptionHandler::BeforeUnhandledException::add(UnhandledExceptionDelegate^ value)
		{
			beforeUnhandledExceptionEvent += value;
		}

		inline void ExceptionHandler::BeforeUnhandledException::remove(UnhandledExceptionDelegate^ value)
		{
			beforeUnhandledExceptionEvent -= value;
		}

		inline void ExceptionHandler::AfterUnhandledException::add(UnhandledExceptionDelegate^ value)
		{
			afterUnhandledExceptionEvent += value;
		}

		inline void ExceptionHandler::AfterUnhandledException::remove(UnhandledExceptionDelegate^ value)
		{
			afterUnhandledExceptionEvent -= value;
		}

		inline String^ ExceptionHandler::AppName::get(void)
		{
			return gcnew String(BT_GetAppName());
		}

		inline void ExceptionHandler::AppName::set(String^ value)
		{
			pin_ptr<const wchar_t> wstrAppName(PtrToStringChars(value));
			BT_SetAppName(wstrAppName);
		}

		inline String^ ExceptionHandler::AppVersion::get(void)
		{
			return gcnew String(BT_GetAppVersion());
		}

		inline void ExceptionHandler::AppVersion::set(String^ value)
		{
			pin_ptr<const wchar_t> wstrAppVersion(PtrToStringChars(value));
			BT_SetAppVersion(wstrAppVersion);
		}

		inline void ExceptionHandler::ReadVersionInfo(Assembly^ assembly)
		{
			ReadVersionInfo(assembly->GetName());
		}

		inline void ExceptionHandler::ReadVersionInfo(void)
		{
			ReadVersionInfo(Assembly::GetEntryAssembly());
		}

		inline String^ ExceptionHandler::GetDialogMessage(DialogMessageType dlgMsg)
		{
			return gcnew String(BT_GetDialogMessage((BUGTRAP_DIALOGMESSAGE)dlgMsg));
		}

		inline void ExceptionHandler::SetDialogMessage(DialogMessageType dlgMsg, String^ value)
		{
			pin_ptr<const wchar_t> wstrMessage(PtrToStringChars(value));
			BT_SetDialogMessage((BUGTRAP_DIALOGMESSAGE)dlgMsg, wstrMessage);
		}

		inline String^ ExceptionHandler::SupportURL::get(void)
		{
			return gcnew String(BT_GetSupportURL());
		}

		inline void ExceptionHandler::SupportURL::set(String^ value)
		{
			pin_ptr<const wchar_t> wstrURL(PtrToStringChars(value));
			BT_SetSupportURL(wstrURL);
		}

		inline String^ ExceptionHandler::SupportEMail::get(void)
		{
			return gcnew String(BT_GetSupportEMail());
		}

		inline void ExceptionHandler::SupportEMail::set(String^ value)
		{
			pin_ptr<const wchar_t> wstrEMail(PtrToStringChars(value));
			BT_SetSupportEMail(wstrEMail);
		}

		inline String^ ExceptionHandler::SupportHost::get(void)
		{
			return gcnew String(BT_GetSupportHost());
		}

		inline void ExceptionHandler::SupportHost::set(String^ value)
		{
			pin_ptr<const wchar_t> wstrHost(PtrToStringChars(value));
			BT_SetSupportHost(wstrHost);
		}

		inline short ExceptionHandler::SupportPort::get(void)
		{
			return BT_GetSupportPort();
		}

		inline void ExceptionHandler::SupportPort::set(short value)
		{
			BT_SetSupportPort(value);
		}

		inline String^ ExceptionHandler::NotificationEMail::get(void)
		{
			return gcnew String(BT_GetNotificationEMail());
		}

		inline void ExceptionHandler::NotificationEMail::set(String^ value)
		{
			pin_ptr<const wchar_t> wstrEMail(PtrToStringChars(value));
			BT_SetNotificationEMail(wstrEMail);
		}

		inline FlagsType ExceptionHandler::Flags::get(void)
		{
			return (FlagsType)BT_GetFlags();
		}

		inline void ExceptionHandler::Flags::set(FlagsType value)
		{
			BT_SetFlags((DWORD)value);
		}

		inline MinidumpType ExceptionHandler::DumpType::get(void)
		{
			return (MinidumpType)BT_GetDumpType();
		}

		inline void ExceptionHandler::DumpType::set(MinidumpType value)
		{
			BT_SetDumpType((DWORD)value);
		}

		inline ReportFormatType ExceptionHandler::ReportFormat::get(void)
		{
			return (ReportFormatType)BT_GetReportFormat();
		}

		inline void ExceptionHandler::ReportFormat::set(ReportFormatType value)
		{
			BT_SetReportFormat((BUGTRAP_REPORTFORMAT)value);
		}

		inline String^ ExceptionHandler::UserMessage::get(void)
		{
			return gcnew String(BT_GetUserMessage());
		}

		inline void ExceptionHandler::UserMessage::set(String^ value)
		{
			pin_ptr<const wchar_t> wstrMessage(PtrToStringChars(value));
			BT_SetUserMessage(wstrMessage);
		}

		inline ActivityType ExceptionHandler::Activity::get(void)
		{
			return (ActivityType)BT_GetActivityType();
		}

		inline void ExceptionHandler::Activity::set(ActivityType value)
		{
			BT_SetActivityType((BUGTRAP_ACTIVITY)value);
		}

		inline String^ ExceptionHandler::ReportFilePath::get(void)
		{
			return gcnew String(BT_GetReportFilePath());
		}

		inline void ExceptionHandler::ReportFilePath::set(String^ value)
		{
			pin_ptr<const wchar_t> wstrPath(PtrToStringChars(value));
			BT_SetReportFilePath(wstrPath);
		}

		inline String^ ExceptionHandler::MailProfile::get(void)
		{
			return gcnew String(BT_GetMailProfile());
		}

		inline void ExceptionHandler::SetMailProfile(String^ profile, String^ password)
		{
			pin_ptr<const wchar_t> wstrProfile(PtrToStringChars(profile));
			pin_ptr<const wchar_t> wstrPassword(PtrToStringChars(password));
			BT_SetMailProfile(wstrProfile, wstrPassword);
		}

		inline void ExceptionHandler::ExportRegKey(String^ fileName, String^ key)
		{
			pin_ptr<const wchar_t> wstrFileName(PtrToStringChars(fileName));
			pin_ptr<const wchar_t> wstrKey(PtrToStringChars(key));
			BT_ExportRegistryKey(wstrFileName, wstrKey);
		}

		inline void ExceptionHandler::MakeSnapshot(String^ fileName)
		{
			pin_ptr<const wchar_t> wstrFileName(PtrToStringChars(fileName));
			BT_MakeSnapshot(wstrFileName);
		}

		inline LogFile::LogFile(void)
		{
			Detach();
		}

		inline LogFile::LogFile(IntPtr handle)
		{
			this->defaultLogLevel = LogLevelType::Info;
			this->handle = handle;
		}

		inline LogFile::LogFile(String^ fileName, LogFormatType logFormat)
		{
			this->defaultLogLevel = LogLevelType::Info;
			PrvOpen(fileName, logFormat);
		}

		inline LogFile::~LogFile()
		{
			this->!LogFile();
		}

		inline LogFile::!LogFile()
		{
			Close();
		}

		inline void LogFile::Open(String^ fileName, LogFormatType logFormat)
		{
			Close();
			PrvOpen(fileName, logFormat);
		}

		inline void LogFile::Detach(void)
		{
			this->defaultLogLevel = LogLevelType::Info;
			this->handle = IntPtr::Zero;
		}

		inline LogLevelType LogFile::DefaultLogLevel::get(void)
		{
			return this->defaultLogLevel;
		}

		inline void LogFile::DefaultLogLevel::set(LogLevelType value)
		{
			this->defaultLogLevel = value;
		}

		inline IntPtr LogFile::Handle::get(void)
		{
			return this->handle;
		}

		inline void LogFile::Insert(String^ entry)
		{
			Insert(this->defaultLogLevel, entry);
		}

		inline void LogFile::Insert(String^ format, ... array<Object^>^ args)
		{
			Insert(this->defaultLogLevel, format, args);
		}

		inline void LogFile::Append(String^ entry)
		{
			Append(this->defaultLogLevel, entry);
		}

		inline void LogFile::Append(String^ format, ... array<Object^>^ args)
		{
			Append(this->defaultLogLevel, format, args);
		}
	}
}

#pragma unmanaged

#endif // _MANAGED
