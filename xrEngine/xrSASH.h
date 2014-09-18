#ifndef	xrSASH_included
#define	xrSASH_included
#pragma once

#include "OpenAutomate/OpenAutomate.h"

//struct oaOptionDependencyStruct;
//typedef struct oaOptionDependencyStruct oaOptionDependency;

//struct oaNamedOptionStruct;
//typedef struct oaNamedOptionStruct oaNamedOption;

class ENGINE_API xrSASH
{
public:
	xrSASH();
	~xrSASH();

	//	Execution control
	bool	Init(const char* pszParam);
	void	MainLoop();

	bool	IsRunning() { return m_bRunning;}
	bool	IsBenchmarkRunning() { return m_bBenchmarkRunning;}

	//	Event handlers
	void	StartBenchmark();
	void	DisplayFrame(float t);
	void	EndBenchmark();

	//	Error report
	void	OnConsoleInvalidSyntax(bool bLastLine, const char * pszMsg, ...);

private:
	//	Internal loops
	void	LoopOA();
	void	LoopNative();

	//	Native specific
	void	ReportNative( LPCSTR pszTestName);

	//	OA command handlers
	void	GetAllOptions();
	void	GetCurrentOptions();
	void	SetOptions();
	void	GetBenchmarks();
	void	RunBenchmark( LPCSTR pszName);

	//	Effectively restores/releases some engine systems
	void	TryInitEngine( bool bNoRun = true );
	void	ReleaseEngine();

	//	OA option handling
	void	DescribeOption( char* pszOptionName, const oaOptionDependency &Dependency);
	oaOptionDataType
			GetOptionType( char* pszOptionName );
	void	GetOption( char* pszOptionName);
	void	SetOption(oaNamedOption *pOption);

	//	OA Error report
	void	Message( oaErrorType MessageType, const char * pszMsg);
	void	Message( oaErrorType MessageType, const char * pszMsg, va_list &mark);

private:
	//	States
	bool	m_bInited;
	bool	m_bOpenAutomate;
	bool	m_bRunning;
	bool	m_bBenchmarkRunning;
	bool	m_bReinitEngine;
	//	Guards
	bool	m_bExecutingConsoleCommand;	//	Guard to pass to OA only those command that were issued by OA
	//	Native benchmarking
	string64	m_strBenchCfgName;
	CTimer		m_FrameTimer;
	xr_vector<float>	m_aFrimeTimes;
};

extern xrSASH ENGINE_API g_SASH;

#endif	//	xrSASH_included