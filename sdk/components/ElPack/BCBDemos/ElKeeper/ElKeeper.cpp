//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("ElKeeper.res");
USERES("LOGOSTRS.RES");
USEFORMNS("ABOUT.PAS", About, AboutBox);
USEUNIT("Cryptcon.pas");
USEFORMNS("frmPassword.pas", Frmpassword, PasswordDlg);
USEFORMNS("frmPswGen.pas", Frmpswgen, PswGenForm);
USEFORMNS("frmRecProp.pas", Frmrecprop, RecPropsForm);
USEFORMNS("frmQuickAccess.pas", Frmquickaccess, QuickAccessForm);

USEUNIT("IDEAUnit.pas");
USEUNIT("KeeperOpts.pas");
USEFORMNS("LogoMain.pas", Logomain, LogoAppForm);
USEUNIT("LogoStrs.pas");
USEUNIT("md5unit.pas");
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
//      Application->Initialize();
        Application->Title = "EldoS Keeper";
        Application->HelpFile = "ElKeeper.hlp";
        Application->CreateForm(__classid(TLogoAppForm), &LogoAppForm);
        Application->CreateForm(__classid(TPasswordDlg), &PasswordDlg);
        Application->CreateForm(__classid(TQuickAccessForm), &QuickAccessForm);
        Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------
