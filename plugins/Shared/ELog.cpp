//----------------------------------------------------
// file: NetDeviceELog.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "ELog.h"
#ifdef _EDITOR
	#include "LogForm.h"
	#include "ui_main.h"
	void __stdcall ELogCallback(LPCSTR txt)
	{
    	if (0==txt[0]) return;
    	bool bDlg 		= ('#'==txt[0])||((0!=txt[1])&&('#'==txt[1]));
        TMsgDlgType mt	= ('!'==txt[0])||((0!=txt[1])&&('!'==txt[1]))?mtError:mtInformation;
        if (('!'==txt[0])||('#'==txt[0])) txt++;
        if (('!'==txt[0])||('#'==txt[0])) txt++;
		if (bDlg)		TfrmLog::AddDlgMessage	(mt,txt);
        else			TfrmLog::AddMessage		(mt,txt);
	}
#endif
#ifdef _LW_EXPORT
	#include <lwhost.h>
	extern "C" LWMessageFuncs	*g_msg;
	void ELogCallback(LPCSTR txt)
	{
		if (0==txt[0])	return;
		bool bDlg 		= ('#'==txt[0])||((0!=txt[1])&&('#'==txt[1]));
		if (bDlg){
			int mt		= ('!'==txt[0])||((0!=txt[1])&&('!'==txt[1]))?1:0;
			if (('!'==txt[0])||('#'==txt[0])) txt++;
			if (('!'==txt[0])||('#'==txt[0])) txt++;
			if (mt==1)	g_msg->error(txt,0);
			else		g_msg->info(txt,0);
		}
	}
#endif
#ifdef _MAX_EXPORT
	#include "..\Max\Export\NetDeviceLog.h"
	void ELogCallback(LPCSTR txt)
	{
 		if (0!=txt[0]){
			if (txt[0]=='!')EConsole.print(mtError,txt+1);
			else			EConsole.print(mtInformation,txt);
		}
	}
#endif
#ifdef _MAYA_PLUGIN
	void ELogCallback(LPCSTR txt)
	{
 		if (0!=txt[0]){
			if (txt[0]=='!')std::cerr << "XR-Error: " << txt+1 << "\n";
			else			std::cerr << "XR-Info: " << txt << "\n";
		}
//.		MStringArray res;
//		MGlobal::executeCommand("confirmDialog -title \"Error\" -message \"Mesh have non-triangulated polygon.\" -button \"Ok\" -defaultButton \"Ok\"",res);
	}
#endif

//----------------------------------------------------
CLog ELog;
//----------------------------------------------------

int CLog::DlgMsg (TMsgDlgType mt, TMsgDlgButtons btn, LPCSTR _Format, ...)
{
    in_use = true;
	char buf[4096];
	va_list l;
	va_start( l, _Format );
	vsprintf( buf, _Format, l );

	int res=0;
#ifdef _EDITOR 
    ExecCommand(COMMAND_RENDER_FOCUS);

    res=MessageDlg(buf, mt, btn, 0);
    if (mtConfirmation==mt){
        switch (res){
        case mrYes: 	strcat(buf," - Yes."); 	break;
        case mrNo: 		strcat(buf," - No."); 	break;
        case mrCancel:  strcat(buf," - Cancel.");	break;
        default: strcat(buf," - Something.");
        }
    }
#endif
#ifdef _LW_EXPORT
	switch(mt){
	case mtError:		g_msg->error(buf,0);	break;
	case mtInformation: g_msg->info(buf,0);		break;
	default:			g_msg->info(buf,0);		break;
	}
#endif
#ifdef _MAX_PLUGIN
	switch(mt){
	case mtError:		MessageBox(0,buf,"Error",		MB_OK|MB_ICONERROR);		break;
	case mtInformation: MessageBox(0,buf,"Information",	MB_OK|MB_ICONINFORMATION);	break;
	default:			MessageBox(0,buf,"Information",	MB_OK|MB_ICONINFORMATION);	break;
	}
#endif

    Msg(mt, buf);

    in_use = false;

    return res;
}


int CLog::DlgMsg (TMsgDlgType mt, LPCSTR _Format, ...)
{
    in_use = true;
	char buf[4096];
	va_list l;
	va_start( l, _Format );
	vsprintf( buf, _Format, l );

    int res=0;
#ifdef _EDITOR
    ExecCommand(COMMAND_RENDER_FOCUS);

    if (mtConfirmation==mt)	res=MessageDlg(buf, mt, TMsgDlgButtons() << mbYes << mbNo << mbCancel, 0);
    else                   	res=MessageDlg(buf, mt, TMsgDlgButtons() << mbOK, 0);

    if (mtConfirmation==mt){
        switch (res){
        case mrYes: 	strcat(buf," - Yes."); 	break;
        case mrNo: 		strcat(buf," - No."); 	break;
        case mrCancel:  strcat(buf," - Cancel.");	break;
        default: strcat(buf," - Something.");
        }
    }
#endif
#ifdef _LW_EXPORT
	switch(mt){
	case mtError:		g_msg->error(buf,0);	break;
	case mtInformation: g_msg->info(buf,0);		break;
	default:			g_msg->info(buf,0);		break;
	}
#endif
#ifdef _MAX_PLUGIN
	switch(mt){
	case mtError:		MessageBox(0,buf,"Error",		MB_OK|MB_ICONERROR);		break;
	case mtInformation: MessageBox(0,buf,"Information",	MB_OK|MB_ICONINFORMATION);	break;
	default:			MessageBox(0,buf,"Information",	MB_OK|MB_ICONINFORMATION);	break;
	}
#endif

    Msg(mt,buf);

    in_use = false;
    
    return res;
}

void CLog::Msg(TMsgDlgType mt, LPCSTR _Format, ...)
{
	char buf[4096];
	va_list l;
	va_start( l, _Format );
	vsprintf( buf, _Format, l );

#ifdef _EDITOR
    TfrmLog::AddMessage(mt,AnsiString(buf));
#endif
#ifdef _MAX_EXPORT
	EConsole.print(mt,buf);
#endif
#ifdef _LW_EXPORT
	switch (mt){
	case mtError: g_msg->error(buf,0); break;
	}
#endif

	::LogExecCB = FALSE;
    ::Msg		(buf);
	::LogExecCB	= TRUE;
}
//----------------------------------------------------
