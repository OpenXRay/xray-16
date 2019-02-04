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
    if (0 == txt[0])
        return;
    bool bDlg = ('#' == txt[0]) || ((0 != txt[1]) && ('#' == txt[1]));
    TMsgDlgType mt = ('!' == txt[0]) || ((0 != txt[1]) && ('!' == txt[1])) ? mtError : mtInformation;
    if (('!' == txt[0]) || ('#' == txt[0]))
        txt++;
    if (('!' == txt[0]) || ('#' == txt[0]))
        txt++;
    if (bDlg)
        TfrmLog::AddDlgMessage(mt, txt);
    else
        TfrmLog::AddMessage(mt, txt);
}
#endif

//----------------------------------------------------
CLog ELog;

//----------------------------------------------------

int CLog::DlgMsg(TMsgDlgType mt, TMsgDlgButtons btn, LPCSTR _Format, ...)
{
    in_use = true;
    char buf[4096];
    va_list l;
    va_start(l, _Format);
    vsprintf(buf, _Format, l);

    int res = 0;
#ifdef _EDITOR
    ExecCommand(COMMAND_RENDER_FOCUS);

    res = MessageDlg(buf, mt, btn, 0);
    if (mtConfirmation == mt)
    {
        switch (res)
        {
        case mrYes: strcat(buf, " - Yes."); break;
        case mrNo: strcat(buf, " - No."); break;
        case mrCancel: strcat(buf, " - Cancel."); break;
        default: strcat(buf, " - Something.");
        }
    }
#endif

    Msg(mt, buf);

    in_use = false;

    return res;
}

int CLog::DlgMsg(TMsgDlgType mt, LPCSTR _Format, ...)
{
    in_use = true;
    char buf[4096];
    va_list l;
    va_start(l, _Format);
    vsprintf(buf, _Format, l);

    int res = 0;
#ifdef _EDITOR
    ExecCommand(COMMAND_RENDER_FOCUS);

    if (mtConfirmation == mt)
        res = MessageDlg(buf, mt, TMsgDlgButtons() << mbYes << mbNo << mbCancel, 0);
    else
        res = MessageDlg(buf, mt, TMsgDlgButtons() << mbOK, 0);

    if (mtConfirmation == mt)
    {
        switch (res)
        {
        case mrYes: strcat(buf, " - Yes."); break;
        case mrNo: strcat(buf, " - No."); break;
        case mrCancel: strcat(buf, " - Cancel."); break;
        default: strcat(buf, " - Something.");
        }
    }
#endif

    Msg(mt, buf);

    in_use = false;

    return res;
}

void CLog::Msg(TMsgDlgType mt, LPCSTR _Format, ...)
{
    char buf[4096];
    va_list l;
    va_start(l, _Format);
    vsprintf(buf, _Format, l);

#ifdef _EDITOR
    TfrmLog::AddMessage(mt, AnsiString(buf));
#endif

    ::LogExecCB = FALSE;
    ::Msg(buf);
    ::LogExecCB = TRUE;
}

//----------------------------------------------------
