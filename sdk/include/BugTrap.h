/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2009 IntelleSoft.
 * All rights reserved.
 *
 * Description: Definitions of external functions.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#ifndef _BUGTRAP_H_
#define _BUGTRAP_H_

#pragma once

#if defined _MANAGED && ! defined _UNICODE
 #error Managed version of BugTrap requires Unicode character set
#endif // _MANAGED && ! _UNICODE

/*
 * The following ifdef block is the standard way of creating macros which make exporting
 * from a DLL simpler. All files within this DLL are compiled with the BUGTRAP_EXPORTS
 * symbol defined on the command line. this symbol should not be defined on any project
 * that uses this DLL. This way any other project whose source files include this file see
 * BUGTRAP_API functions as being imported from a DLL, whereas this DLL sees symbols
 * defined with this macro as being exported.
 */
#ifdef BUGTRAP_EXPORTS
 #define BUGTRAP_API __declspec(dllexport)
#else
 #define BUGTRAP_API __declspec(dllimport)
#endif // ! BUGTRAP_EXPORTS

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @defgroup TypeDef Common type definitions
 * @{
 */

/**
 * @brief Type of action which is performed in response to the error.
 */
typedef enum BUGTRAP_ACTIVITY_tag
{
	/**
	 * @brief Display BugTrap dialog to allow user selecting desirable
	 * option. This is the default option.
	 */
	BTA_SHOWUI       = 1,
	/**
	 * @brief Automatically save error report to file.
	 * Use BT_SetReportFilePath() to specify report path.
	 */
	BTA_SAVEREPORT   = 2,
	/**
	 * @brief Automatically send error report by e-mail.
	 */
	BTA_MAILREPORT   = 3,
	/**
	 * @brief Automatically send bug report to support server.
	 */
	BTA_SENDREPORT   = 4,
	/**
	* @brief Call custom handler specified by BT_SetCustomActivityHandler().
	*/
	BTA_CUSTOM = 5,
}
BUGTRAP_ACTIVITY;

/**
 * @brief Different BugTrap options. You can use any combinations
 * of these flags.
 */
typedef enum BUGTRAP_FLAGS_tag
{
	/**
	 * @brief Equivalent of no options.
	 */
	BTF_NONE           = 0x000,
	/**
	 * @brief In detailed mode BugTrap generates mini-dump and
	 * packs custom log files within the report.
	 */
	BTF_DETAILEDMODE   = 0x001,
	/**
	 * @brief BugTrap may open its own editor for e-mail messages
	 * instead of the editor used by the system. Use this
	 * option if you aren't aware of the type of e-mail
	 * client installed on user computers.
	 */
	BTF_EDITMAIL       = 0x002,
	/**
	 * @brief Specify this option to attach bug report to e-mail
	 * messages. Be careful with this option. It's potentially
	 * dangerous for the detailed mode because of even zipped
	 * mini-dump may require huge size. It may exceed the
	 * maximum size of e-mail message supported by Internet
	 * provider.
	 */
	BTF_ATTACHREPORT   = 0x004,
	/**
	 * @brief Set this flag to add list of all processes and loaded
	 * modules to the report. Disable this option to speedup report
	 * generation.
	 */
	BTF_LISTPROCESSES  = 0x008,
	/**
	 * @brief By default BugTrap displays simplified dialog on the
	 * screen allowing user to perform only common actions. Enable
	 * this flag to immediately display dialog with advanced error
	 * information.
	 */
	BTF_SHOWADVANCEDUI = 0x010,
	/**
	 * @brief Bug report in detailed error mode may also include a
	 * screen shot automatically captured by BugTrap. By default this
	 * option is disabled to minimize report size, but it may be useful
	 * if you want to know which dialogs were shown on the screen.
	 */
	BTF_SCREENCAPTURE  = 0x020,
#ifdef _MANAGED
	/**
	 * @brief Generate native stack trace and modules information along
	 * with managed exception information. Disable this option to speedup
	 * report generation.
	 */
	 BTF_NATIVEINFO    = 0x040,
#endif // _MANAGED
	 /**
	  * @brief When enabled, BugTrap injects fake SetUnhandledExceptionFilter()
	  * to handle the most severe C runtime errors. Usually such errors
	  * are not being reported to custom unhandled exception filters. This
	  * flag vanishes any attempts of C runtime library to override
	  * unhandled exception filter defined in BugTrap. This option should
	  * be used with caution, because such technique may be incompatible
	  * with future Windows versions.
	  */
	 BTF_INTERCEPTSUEF = 0x080,
	 /**
	  * @brief When report is being sent over TCP/HTTP protocol, BugTrap could
	  * ask user to provide verbal description of the problem if this option is
	  * enabled. BugTrap always prompts user to describe error for reports
	  * transmitted over email regardless this option.
	  */
	 BTF_DESCRIBEERROR = 0x100,
	 /**
	  * @brief Automatically restart the application after the crash has been
	  * handled.
	  */
	 BTF_RESTARTAPP    = 0x200,
     /**
      * @brief By default BugTrap show a "More..." button to open
      * the advanced ui in the simple dialog. Enable this flag
      * hide the button.
      */
     BTF_HIDEMOREBUTTON = 0x400,
}
BUGTRAP_FLAGS;

/**
 * @brief These flags control application termination mode:
 * whatever the application needs to be terminated after error handling,
 * control should passed to another exception handler.
 */
typedef enum BUGTRAP_EXITMODE_tag
{
	/**
	 * @brief BugTrap executes desirable action (usually displays the dialog).
	 * The application is being terminated when user closes the dialog. This is default action.
	 */
	BTEM_TERMINATEAPP,
	/**
	 * @brief BugTrap executes desirable action and continues to search up the stack for a handler.
	 */
	BTEM_CONTINUESEARCH,
	/**
	 * @brief BugTrap executes desirable action and transfers control to the exception handlers.
	 */
	BTEM_EXECUTEHANDLER
}
BUGTRAP_EXITMODE;

/**
 * @brief Set of available log levels.
 */
typedef enum BUGTRAP_LOGLEVEL_tag
{
	/**
	 * @brief All message levels are prohibited.
	 */
	BTLL_NONE      = 0,
	/**
	 * @brief Error message.
	 */
	BTLL_ERROR     = 1,
	/**
	 * @brief Waning message.
	 */
	BTLL_WARNING   = 2,
	/**
	 * @brief Important information message.
	 */
	BTLL_IMPORTANT = 3,
	/**
	 * @brief Regular information message.
	 */
	BTLL_INFO      = 4,
	/**
	 * @brief Verbose information message.
	 */
	BTLL_VERBOSE   = 5,
	/**
	 * @brief All levels of messages are accepted.
	 */
	BTLL_ALL       = BTLL_INFO
}
BUGTRAP_LOGLEVEL;

/**
 * @brief Type of log echo mode.
 */
typedef enum BUGTRAP_LOGECHOTYPE_tag
{
	/**
	 * @brief Do not duplicate log output to file.
	 */
	BTLE_NONE   = 0x00,
	/**
	 * @brief Send a copy of log output to STDOUT (mutually exclusive with STDERR mode).
	 */
	BTLE_STDOUT = 0x01,
	/**
	 * @brief Send a copy of log output to STDERR (mutually exclusive with STDOUT mode).
	 */
	BTLE_STDERR = 0x02,
	/**
	 * @brief Send a copy of log output to the debugger (can accompany STDOUT or STDERR mode).
	 */
	BTLE_DBGOUT = 0x04
}
BUGTRAP_LOGECHOTYPE;

/**
 * @brief Set of log file options.
 */
typedef enum BUGTRAP_LOGFLAGS_tag
{
	/**
	 * @brief Do not show any additional entries in the log.
	 */
	BTLF_NONE          = 0x00,
	/**
	 * @brief Use this option if you want to store message levels in a file.
	 */
	BTLF_SHOWLOGLEVEL  = 0x01,
	/**
	 * @brief Use this option if you want to store message timestamps in a file.
	 * Timestamps are stored in universal (locale independent) format: YYYY/MM/DD HH:MM:SS
	 */
	BTLF_SHOWTIMESTAMP = 0x02
}
BUGTRAP_LOGFLAGS;

/**
 * @brief Format of error report.
 */
typedef enum BUGTRAP_REPORTFORMAT_tag
{
	/**
	 * @brief Report stored in structured XML format.
	 */
	BTRF_XML  = 1,
	/**
	 * @brief Report stored in plain text format.
	 */
	BTRF_TEXT = 2
}
BUGTRAP_REPORTFORMAT;

/**
 * @brief Format of log file.
 */
typedef enum BUGTRAP_LOGFORMAT_tag
{
	/**
	 * @brief Log stored in structured XML format.
	 * Log data is entirely stored in memory until application exists or crashes.
	 * It is designed for fast IO and it cannot handle large logs.
	 */
	BTLF_XML    = 1,
	/**
	 * @brief Log stored in plain text format.
	 * Log data is entirely stored in memory until application exists or crashes.
	 * It is designed for fast IO and it cannot handle large logs.
	 */
	BTLF_TEXT   = 2,
	/**
	 * @brief Log stored in plain text format.
	 * Unlike @a BTLF_XML and @a BTLF_TEXT log data is not cached in memory.
	 * This type of log is optimized for large logs.
	 */
	BTLF_STREAM = 3
}
BUGTRAP_LOGFORMAT;

/**
 * @brief Type of user defined message displayed on the screen.
 */
typedef enum BUGTRAP_DIALOGMESSAGE_tag
{
	/**
	 * @brief 1st line of introduction message.
	 */
	BTDM_INTRO1 = 1,
	/**
	 * @brief A bunch of lines of introduction message.
	 */
	BTDM_INTRO2 = 2
}
BUGTRAP_DIALOGMESSAGE;

/**
 * @brief Type of log file attached to the report.
 */
typedef enum BUGTRAP_LOGTYPE_tag
{
	/**
	 * @brief Regular log file maintained by the user.
	 */
	BTLT_LOGFILE,
	/**
	 * @brief Log file that's exported from the registry.
	 */
	BTLT_REGEXPORT
}
BUGTRAP_LOGTYPE;

/**
 * @brief Log file entry structure return by BT_GetLogFileEntry() for regular log files.
 */
typedef struct BUGTRAP_LOGFILEENTRY_tag
{
	/**
	 * @brief Log file name.
	 */
	TCHAR szLogFileName[MAX_PATH];
}
BUGTRAP_LOGFILEENTRY;

/**
 * @brief Log file entry structure return by BT_GetLogFileEntry() for log files exported from the registry.
 */
typedef struct BUGTRAP_REGEXPORTENTRY_tag
{
	/**
	 * @brief Log file name.
	 */
	TCHAR szLogFileName[MAX_PATH];
	/**
	 * @brief Exported registry key.
	 */
	TCHAR szRegKey[MAX_PATH];
}
BUGTRAP_REGEXPORTENTRY;

/**
 * @brief Type definition of user-defined error handler that's called before and after main BugTrap dialog.
 */
typedef void (CALLBACK * BT_ErrHandler)(INT_PTR nErrHandlerParam);

/**
* @brief Type definition of user-defined activity handler.
*/
typedef void (CALLBACK * BT_CustomActivityHandler)(LPCTSTR pszReportFilePath,  INT_PTR nCustomActivityHandlerParam);

/** @} */

/**
 * @defgroup AppTitle Application title management
 * @{
 */

/**
 * @brief Get application name.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetAppName(void);
/**
 * @brief Set application name of the project where BugTrap is used.
 */
BUGTRAP_API void APIENTRY BT_SetAppName(LPCTSTR pszAppName);
/**
 * @brief Get application version number.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetAppVersion(void);
/**
 * @brief Set application version number.
 */
BUGTRAP_API void APIENTRY BT_SetAppVersion(LPCTSTR pszAppVersion);
/**
 * @brief Read application name and version number from version info block.
 * @note @a hModule can be set to NULL for the main executable.
 */
BUGTRAP_API BOOL APIENTRY BT_ReadVersionInfo(HMODULE hModule);
/** @} */

/**
 * @defgroup ModuleOfInterest Module of interest
 * Only exceptions that contain the module of interest at the stack trace will
 * be processed. If no module of interest set, all exceptions will be processed.
 * @{
 */

/**
 * @brief Get module of interest
 */
BUGTRAP_API HMODULE APIENTRY BT_GetModule();
/**
 * @brief Set module of interest
 */
BUGTRAP_API void APIENTRY BT_SetModule(HMODULE hModule);
/** @} */

/**
 * @defgroup UserInterface User interface customization.
 * @{
 */
/**
 * @brief Get user defined message displayed on the screen.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetDialogMessage(BUGTRAP_DIALOGMESSAGE eDialogMessage);
/**
 * @brief Set user defined message displayed on the screen.
 */
BUGTRAP_API void APIENTRY BT_SetDialogMessage(BUGTRAP_DIALOGMESSAGE eDialogMessage, LPCTSTR pszMessage);
/** @} */


/**
 * @defgroup SupportURL Support URL management
 * @{
 */

/**
 * @brief Get HTTP address of product support site.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetSupportURL(void);
/**
 * @brief Set HTTP address of product support site.
 */
BUGTRAP_API void APIENTRY BT_SetSupportURL(LPCTSTR pszSupportURL);
/**
 * @brief Get product support e-mail address.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetSupportEMail(void);
/**
 * @brief Set product support e-mail address.
 */
BUGTRAP_API void APIENTRY BT_SetSupportEMail(LPCTSTR pszSupportEMail);

/** @} */

/**
 * @defgroup SrvConfig Server configuration
 * @{
 */

/// Use this port for HTTP protocol.
#define BUGTRAP_HTTP_PORT             80

/**
 * @brief Get host name of BugTrap Server. This server can automatically
 * gather bug reports for the application.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetSupportHost(void);
/**
 * @brief Set host name (address) of BugTrap Server. This server
 * can automatically gather bug reports for the application.
 */
BUGTRAP_API void APIENTRY BT_SetSupportHost(LPCTSTR pszSupportHost);
/**
 * @brief Get port number of BugTrap Server. This server can automatically
 * gather bug reports for the application.
 */
BUGTRAP_API SHORT APIENTRY BT_GetSupportPort(void);
/**
 * @brief Set port number of BugTrap Server. This server
 * can automatically gather bug reports for the application.
 */
BUGTRAP_API void APIENTRY BT_SetSupportPort(SHORT nSupportPort);
/**
 * @brief Set host name (address) and port number of BugTrap Server. This server
 * can automatically gather bug reports for the application.
 */
BUGTRAP_API void APIENTRY BT_SetSupportServer(LPCTSTR pszSupportHost, SHORT nSupportPort);
/**
 * @brief Get error notification e-mail address.
 * BugTrap Server may automatically notify product support by e-mail about new bug reports.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetNotificationEMail(void);
/**
 * @brief Set error notification e-mail address.
 * BugTrap Server may automatically notify product support by e-mail about new bug reports.
 */
BUGTRAP_API void APIENTRY BT_SetNotificationEMail(LPCTSTR pszNotificationEMail);

/** @} */

/**
 * @defgroup CustomReport Report customization
 * @{
 */

/// Disables mini-dump generation in detailed mode.
#define MiniDumpNoDump                MAXDWORD

/**
 * @brief Get current BugTrap options.
 */
BUGTRAP_API DWORD APIENTRY BT_GetFlags(void);
/**
 * @brief Set various BugTrap options.
 */
BUGTRAP_API void APIENTRY BT_SetFlags(DWORD dwFlags);
/**
 * @brief Get the type of produced mini-dump in detailed mode.
 */
BUGTRAP_API DWORD APIENTRY BT_GetDumpType(void);
/**
 * @brief Set the type of produced mini-dump in detailed mode.
 */
BUGTRAP_API void APIENTRY BT_SetDumpType(DWORD dwDumpType);
/**
 * @brief Get format of error report.
 */
BUGTRAP_API BUGTRAP_REPORTFORMAT APIENTRY BT_GetReportFormat(void);
/**
 * @brief Set format of error report.
 */
BUGTRAP_API void APIENTRY BT_SetReportFormat(BUGTRAP_REPORTFORMAT eReportFormat);
/**
 * @brief Get user defined message. This message may be printed to a log file.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetUserMessage(void);
/**
 * @brief Set user defined message. This message may be printed to a log file.
 */
BUGTRAP_API void APIENTRY BT_SetUserMessage(LPCTSTR pszUserMessage);
/**
 * @brief Set user defined message. This message may be printed to a log file.
 */
BUGTRAP_API void APIENTRY BT_SetUserMessageFromCode(DWORD dwErrorCode);

/** @} */

/**
 * @defgroup SilentFunc Silent mode configuration
 * @{
 */

/**
 * @brief Get the type of action which is performed in response to the error.
 */
BUGTRAP_API BUGTRAP_ACTIVITY APIENTRY BT_GetActivityType(void);
/**
 * @brief Set the type of action which is performed in response to the error.
 */
BUGTRAP_API void APIENTRY BT_SetActivityType(BUGTRAP_ACTIVITY eActivityType);
/**
 * @brief Get application termination mode.
 */
BUGTRAP_API BUGTRAP_EXITMODE APIENTRY BT_GetExitMode(void);
/**
 * @brief Set application termination mode.
 */
BUGTRAP_API void APIENTRY BT_SetExitMode(BUGTRAP_EXITMODE eExitMode);
/**
 * @brief Get the path of error report.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetReportFilePath(void);
/**
 * @brief Set the path of error report.
 * This function has effect only for @a BTA_SAVEREPORT activity.
 */
BUGTRAP_API void APIENTRY BT_SetReportFilePath(LPCTSTR pszReportFilePath);
/**
 * @brief Get the name of MAPI profile.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetMailProfile(void);
/**
 * @brief Set the name and password of MAPI profile.
 */
BUGTRAP_API void APIENTRY BT_SetMailProfile(LPCTSTR pszMailProfile, LPCTSTR pszMailPassword);

/** @} */

/**
 * @defgroup LogFiles Custom log files management
 * @{
 */

/**
 * @brief Export registry key to the file.
 */
BUGTRAP_API int APIENTRY BT_ExportRegistryKey(LPCTSTR pszRegFile, LPCTSTR pszRegKey);
/**
 * @brief Add custom log entry to the list of custom log files
 * attached to bug report.
 */
BUGTRAP_API void APIENTRY BT_AddLogFile(LPCTSTR pszLogFile);
/**
 * @brief Add custom log entry to the list of custom log files
 * attached to bug report.
 */
BUGTRAP_API void APIENTRY BT_AddRegFile(LPCTSTR pszRegFile, LPCTSTR pszRegKey);
/**
 * @brief Delete custom log entry from the list of custom log files
 * attached to bug report.
 */
BUGTRAP_API void APIENTRY BT_DeleteLogFile(LPCTSTR pszLogFile);
/**
 * @brief Clear the list of custom log files attached to bug report.
 */
BUGTRAP_API void APIENTRY BT_ClearLogFiles(void);
/**
 * @brief Get number of log files attached to the report.
 */
BUGTRAP_API DWORD APIENTRY BT_GetLogFilesCount(void);
/**
 * @brief This function enumerates log file entries that will be attached to the report.
 */
BUGTRAP_API DWORD APIENTRY BT_GetLogFileEntry(INT_PTR nLogFileIndexOrName, BOOL bGetByIndex, BUGTRAP_LOGTYPE* peLogType, PDWORD pdwLogEntrySize, PVOID pLogEntry);

/** @} */

/**
 * @defgroup CustomErrHandlers Custom error handlers
 * @{
 */

/**
 * @brief Get address of error handler called before BugTrap dialog.
 */
BUGTRAP_API BT_ErrHandler APIENTRY BT_GetPreErrHandler(void);
/**
 * @brief Set address of error handler called before BugTrap dialog.
 */
BUGTRAP_API void APIENTRY BT_SetPreErrHandler(BT_ErrHandler pfnPreErrHandler, INT_PTR nPreErrHandlerParam);
/**
 * @brief Get address of error handler called after BugTrap dialog.
 */
BUGTRAP_API BT_ErrHandler APIENTRY BT_GetPostErrHandler(void);
/**
 * @brief Set address of error handler called after BugTrap dialog.
 */
BUGTRAP_API void APIENTRY BT_SetPostErrHandler(BT_ErrHandler pfnPostErrHandler, INT_PTR nPostErrHandlerParam);

/** @} */

/**
* @defgroup CustomActivityHandlers Custom activity handlers
* @{
*/

/**
* @brief Get address of custom activity handler called at processing BugTrap action.
*/
BUGTRAP_API BT_CustomActivityHandler APIENTRY BT_GetCustomActivityHandler(void);
/**
* @brief Set address of custom activity handler called at processing BugTrap action.
*/
BUGTRAP_API void APIENTRY BT_SetCustomActivityHandler(BT_CustomActivityHandler pfnCustomActivityHandler, INT_PTR nCustomActivityHandlerParam);

/** @} */

/**
 * @defgroup TracingFunc Tracing functions
 * @{
 */

/**
 * @brief Open custom log file. This function is thread safe.
 */
BUGTRAP_API INT_PTR APIENTRY BT_OpenLogFile(LPCTSTR pszLogFileName, BUGTRAP_LOGFORMAT eLogFormat);
/**
 * @brief Close custom log file. This function is thread safe.
 */
BUGTRAP_API BOOL APIENTRY BT_CloseLogFile(INT_PTR iHandle);
/**
 * @brief Flush contents of the log file.
 * @note This function is optional and not required in normal conditions.
 */
BUGTRAP_API BOOL APIENTRY BT_FlushLogFile(INT_PTR iHandle);
/**
 * @brief Get custom log file name. This function is thread safe.
 */
BUGTRAP_API LPCTSTR APIENTRY BT_GetLogFileName(INT_PTR iHandle);
/**
 * @brief Get maximum log file size in records. This function is thread safe.
 */
BUGTRAP_API DWORD APIENTRY BT_GetLogSizeInEntries(INT_PTR iHandle);
/**
 * @brief Set maximum log file size in records. This function is thread safe.
 */
BUGTRAP_API BOOL APIENTRY BT_SetLogSizeInEntries(INT_PTR iHandle, DWORD dwLogSizeInEntries);
/**
 * @brief Get maximum log file size in bytes. This function is thread safe.
 */
BUGTRAP_API DWORD APIENTRY BT_GetLogSizeInBytes(INT_PTR iHandle);
/**
 * @brief Set maximum log file size in bytes. This function is thread safe.
 */
BUGTRAP_API BOOL APIENTRY BT_SetLogSizeInBytes(INT_PTR iHandle, DWORD dwLogSizeInEntries);
/**
 * @brief Return true if time stamp is added to every log entry.
 */
BUGTRAP_API DWORD APIENTRY BT_GetLogFlags(INT_PTR iHandle);
/**
 * @brief Set true if time stamp is added to every log entry.
 */
BUGTRAP_API BOOL APIENTRY BT_SetLogFlags(INT_PTR iHandle, DWORD dwLogFlags);
/**
 * @brief Return minimal log level accepted by tracing functions.
 */
BUGTRAP_API BUGTRAP_LOGLEVEL APIENTRY BT_GetLogLevel(INT_PTR iHandle);
/**
 * @brief Set minimal log level accepted by tracing functions.
 */
BUGTRAP_API BOOL APIENTRY BT_SetLogLevel(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel);
/**
 * @brief Get echo mode.
 */
BUGTRAP_API DWORD APIENTRY BT_GetLogEchoMode(INT_PTR iHandle);
/**
 * @brief Set echo mode.
 */
BUGTRAP_API BOOL APIENTRY BT_SetLogEchoMode(INT_PTR iHandle, DWORD dwLogEchoMode);
/**
 * @brief Clear log file. This function is thread safe.
 */
BUGTRAP_API BOOL APIENTRY BT_ClearLog(INT_PTR iHandle);
/**
 * @brief Insert entry into the beginning of custom log file. This function is thread safe.
 */
BUGTRAP_API BOOL CDECL BT_InsLogEntryF(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszFormat, ...);
/**
 * @brief Append entry to the end of custom log file. This function is thread safe.
 */
BUGTRAP_API BOOL CDECL BT_AppLogEntryF(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszFormat, ...);
/**
 * @brief Insert entry into the beginning of custom log file. This function is thread safe.
 */
BUGTRAP_API BOOL APIENTRY BT_InsLogEntryV(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszFormat, va_list argList);
/**
 * @brief Append entry to the end of custom log file. This function is thread safe.
 */
BUGTRAP_API BOOL APIENTRY BT_AppLogEntryV(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszFormat, va_list argList);
/**
 * @brief Insert entry into the beginning of custom log file. This function is thread safe.
 */
BUGTRAP_API BOOL APIENTRY BT_InsLogEntry(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszEntry);
/**
 * @brief Append entry to the end of custom log file. This function is thread safe.
 */
BUGTRAP_API BOOL APIENTRY BT_AppLogEntry(INT_PTR iHandle, BUGTRAP_LOGLEVEL eLogLevel, LPCTSTR pszEntry);

/** @} */

/**
 * @defgroup InternalFunc Internal functions
 * @{
 */

/**
 * Explicitly installs BugTrap exception filter.
 * @note Normally you should not call this function.
 */
BUGTRAP_API LPTOP_LEVEL_EXCEPTION_FILTER APIENTRY BT_InstallSehFilter(void);
/**
 * Explicitly uninstalls BugTrap exception filter and restores the previous exception filter.
 * @note Normally you should not call this function.
 */
BUGTRAP_API void APIENTRY BT_UninstallSehFilter(void);
/**
 * @brief Explicitly intercepts SetUnhandledExceptionFilter() in a module.
 * This function could be useful for dynamically loaded modules.
 */
BUGTRAP_API void BT_InterceptSUEF(HMODULE hModule, BOOL bOverride);
/**
 * @brief Take a snapshot of program memory and save it to a file.
 */
BUGTRAP_API BOOL APIENTRY BT_SaveSnapshot(PCTSTR pszFileName);
/**
 * @brief Take a snapshot of program memory and save it to a file.
 */
BUGTRAP_API BOOL APIENTRY BT_SaveSnapshotEx(PEXCEPTION_POINTERS pExceptionPointers, PCTSTR pszFileName);
/**
 * @brief Take a snapshot of program memory and send over network.
 */
BUGTRAP_API BOOL APIENTRY BT_SendSnapshot(void);
/**
 * @brief Take a snapshot of program memory and send over network.
 */
BUGTRAP_API BOOL APIENTRY BT_SendSnapshotEx(PEXCEPTION_POINTERS pExceptionPointers);
/**
 * @brief Take a snapshot of program memory and e-mail it.
 */
BUGTRAP_API BOOL APIENTRY BT_MailSnapshot(void);
/**
 * @brief Take a snapshot of program memory and e-mail it.
 */
BUGTRAP_API BOOL APIENTRY BT_MailSnapshotEx(PEXCEPTION_POINTERS pExceptionPointers);
/**
 * @brief Executes structured exception filter.
 * @note Don't call this function directly in your code.
 */
BUGTRAP_API LONG CALLBACK BT_SehFilter(PEXCEPTION_POINTERS pExceptionPointers);
/**
 * @brief Simulates access violation in C++ application. Used by set_terminate().
 * @note Don't call this function directly in your code.
 */
BUGTRAP_API void CDECL BT_CallSehFilter(void);
/**
 * @brief Executes C++ exception filter.
 * @note Don't call this function directly in your code.
 */
BUGTRAP_API LONG CALLBACK BT_CppFilter(PEXCEPTION_POINTERS pExceptionPointers);
/**
 * @brief Simulates access violation in C++ application. Used by set_terminate().
 * @note Don't call this function directly in your code.
 */
BUGTRAP_API void CDECL BT_CallCppFilter(void);

#ifdef _MANAGED
/**
 * @brief Executes .NET exception filter.
 * @note Don't call this function directly in your code.
 */
BUGTRAP_API LONG CALLBACK BT_NetFilter(PEXCEPTION_POINTERS pExceptionPointers);
/**
 * @brief Simulates access violation in .NET application. Used by set_terminate().
 * @note Don't call this function directly in your code.
 */
BUGTRAP_API void CDECL BT_CallNetFilter(void);
#endif // _MANAGED

/**
 * Installs BugTrap handler to be called by the runtime in response to
 * unhandled C++ exception.
 */
#if defined __cplusplus && defined _INC_EH
 #define BT_SetTerminate() set_terminate(BT_CallCppFilter)
#else
 #define BT_SetTerminate()
#endif // __cplusplus && _INC_EH

/** @} */

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus
 #include "BTTrace.h"

 #if defined __ATLWIN_H__ && defined __AFX_H__
  #define BT_DO_NOT_USE_DEFAULT_NAMESPACES
 #endif // __ATLWIN_H__ && __AFX_H__

 #ifdef __ATLWIN_H__
  #include "BTAtlWindow.h"
 #endif // __ATLWIN_H__

 #ifdef __AFX_H__
  #include "BTMfcWindow.h"
 #endif // __AFX_H__

#endif // __cplusplus

#endif // _BUGTRAP_H_
