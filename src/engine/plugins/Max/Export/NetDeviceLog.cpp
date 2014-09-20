//----------------------------------------------------
// file: NetDeviceLog.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "NetDeviceLog.h"
#include "MeshExpUtility.h"
#include "MeshExpUtility.rh"

//----------------------------------------------------

CExportConsole EConsole;

//----------------------------------------------------
BOOL CALLBACK ConsoleDialogProc( HWND hw, UINT msg, WPARAM wp, LPARAM lp){
	
	std::list<CExportConsole::_ConsoleMsg>::iterator _F;
	std::list<CExportConsole::_ConsoleMsg>::iterator _E;

	switch( msg ){

	case WM_INITDIALOG:
		SetWindowPos(hw,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
		EConsole.m_hWindow = hw;
		EnterCriticalSection( &EConsole.m_CSection );
		if( !EConsole.m_Messages.empty() ){
			_F = EConsole.m_Messages.begin();
			_E = EConsole.m_Messages.end();
			for(;_F!=_E;_F++){
				int k = SendDlgItemMessage( hw, IDC_MESSAGES, LB_ADDSTRING, 0, (LPARAM)_F->buf );
				SendDlgItemMessage( hw, IDC_MESSAGES, LB_SETCURSEL, k, 0 );
			}
			SendDlgItemMessage( hw, IDC_PROGRESS, PBM_SETRANGE,	0, MAKELPARAM(0, 100) );
			SendDlgItemMessage( hw, IDC_PROGRESS, PBM_SETPOS,		0, 0);
		}
		LeaveCriticalSection( &EConsole.m_CSection );
		break;
	case WM_CLOSE:
		SendMessage(U.hPanel,WM_COMMAND,IDC_CLOSE,0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI ConsoleThreadProc( LPVOID ){
	DialogBox( EConsole.m_hInstance,
		MAKEINTRESOURCE(IDD_CONSOLE),
		0/*GetForegroundWindow()*/, ConsoleDialogProc );
	return 0;
}

void CExportConsole::StayOnTop(BOOL flag)
{
	if (flag){	
		SetWindowPos(m_hWindow,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
	}else{	
		SetWindowPos(m_hWindow,HWND_NOTOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
		SetWindowPos(m_hWindow,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
	}
}

bool CExportConsole::Init( HINSTANCE _Inst, HWND _Window ){
	
	if( m_Valid ){
		SetForegroundWindow(m_hWindow);
		return true;}

	m_Enter = false;
	m_hInstance = _Inst;
	m_hParent = _Window;
	InitializeCriticalSection( &m_CSection );
	m_hThread = CreateThread( 0, 0, ConsoleThreadProc, 0, 0, &m_ThreadId );
	m_Valid = true;
	
	return true;
}

void CExportConsole::Clear(){
	
	if( !m_Valid )
		return;

	m_Valid = false;
	TerminateThread( m_hThread, 0 );
	CloseHandle( m_hThread );
	DeleteCriticalSection( &m_CSection );
}

void CExportConsole::print(TMsgDlgType mt, const char *buf){
	_ConsoleMsg msg(buf);

	if( m_Messages.size() > 1000 ) m_Messages.pop_front();
	m_Messages.push_back( buf );

	if( !m_Valid )
		return;

	EnterCriticalSection( &m_CSection );
	if( SendDlgItemMessage( m_hWindow, IDC_MESSAGES, LB_GETCOUNT, 0, 0 ) > 1000 )
		SendDlgItemMessage( m_hWindow, IDC_MESSAGES,LB_DELETESTRING, 0, 0 );
	int k = SendDlgItemMessage( m_hWindow, IDC_MESSAGES,LB_ADDSTRING, 0, (LPARAM)buf );
	SendDlgItemMessage( m_hWindow, IDC_MESSAGES,LB_SETCURSEL, k, 0 );
	DWORD dwCnt = SendMessage	( m_hWindow, LB_GETCOUNT, 0, 0);
	SendMessage	( m_hWindow, LB_SETTOPINDEX, dwCnt-1, 0);
	LeaveCriticalSection( &m_CSection );
}

bool CExportConsole::valid(){
	return m_Valid;
}

CExportConsole::CExportConsole(){
	VERIFY( this == &EConsole );
	m_Valid = false;
}

CExportConsole::~CExportConsole(){
	VERIFY( m_Valid == false );
	m_Messages.clear();
}

//----------------------------------------------------
void CExportConsole::ProgressStart(float max_val, const char* text){
	fMaxVal=max_val;
	fStatusProgress=0;
	Msg(text?text:"");
	ProgressUpdate(0);
}
void CExportConsole::ProgressEnd(){
	ProgressUpdate(0);
}
void CExportConsole::ProgressInc(){
	ProgressUpdate(fStatusProgress+1);
}
void CExportConsole::ProgressUpdate(float val){
	if (_abs(val-fStatusProgress)<1) return;
	fStatusProgress=val;
	EnterCriticalSection( &m_CSection );
    if (fMaxVal>0){
		DWORD progress = (DWORD)((fStatusProgress/fMaxVal)*100);
		SendDlgItemMessage( m_hWindow, IDC_PROGRESS,PBM_SETPOS, progress, 0 );
	}else{
		DWORD progress = 0;
		SendDlgItemMessage( m_hWindow, IDC_PROGRESS,PBM_SETPOS, progress, 0 );
	}
	LeaveCriticalSection( &m_CSection );
}
//----------------------------------------------------


