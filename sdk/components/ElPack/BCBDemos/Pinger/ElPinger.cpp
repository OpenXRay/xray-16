//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("ElPinger.res");
USEFORMNS("MAIN.PAS", Main, MainForm);
USEFORMNS("About.pas", About, AboutBox);
USEFORMNS("frmList.pas", Frmlist, SitesForm);
USEFORMNS("frmProp.pas", Frmprop, PropForm);
USEUNIT("ICMP.pas");
USEUNIT("PingThread.pas");
USEUNIT("SiteMan.pas");
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
//        Application->Initialize();
	    Application->Title = "EldoS Pinger";
        FreeLibrary(GetModuleHandle("OleAut32"));        
        Application->CreateForm(__classid(TMainForm), &MainForm);
        Application->CreateForm(__classid(TSitesForm), &SitesForm);
        Application->CreateForm(__classid(TPropForm), &PropForm);
        Application->CreateForm(__classid(TAboutBox), &AboutBox);
        Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------
