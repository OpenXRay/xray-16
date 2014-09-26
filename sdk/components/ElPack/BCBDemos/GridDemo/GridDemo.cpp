//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("GridDemo.res");
USEFORMNS("Main.pas", Main, Form1);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
        Application->Initialize();
    	Application->Title = "Grid Demo";
        Application->CreateForm(__classid(TForm1), &Form1);
        Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------
