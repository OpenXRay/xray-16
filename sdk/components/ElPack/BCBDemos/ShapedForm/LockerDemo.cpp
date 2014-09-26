//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("LockerDemo.res");
USEFORMNS("LockerForm.pas", Lockerform, frmLocker);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
        Application->Initialize();
        Application->CreateForm(__classid(TfrmLocker), &frmLocker);
        Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------
