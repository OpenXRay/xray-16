//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("ElAppBarDemo.res");
USEFORMNS("frmMain.pas", Frmmain, frmBar);
USEFORMNS("frmOpts.pas", Frmopts, OptionsForm);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
        Application->Initialize();
	Application->Title = "ElAppBar Demo";
        Application->CreateForm(__classid(TfrmBar), &frmBar);
        Application->CreateForm(__classid(TOptionsForm), &OptionsForm);
        Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------
