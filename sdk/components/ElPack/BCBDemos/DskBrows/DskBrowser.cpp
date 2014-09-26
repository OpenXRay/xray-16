//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("DskBrows.res");
USEFORM("frmMain.cpp", MainForm);
USEFORM("frmSearch.cpp", SearchForm);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
        Application->Initialize();
        Application->CreateForm(__classid(TMainForm), &MainForm);
        Application->CreateForm(__classid(TSearchForm), &SearchForm);
        Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------
