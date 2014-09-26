//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("IniEdit.res");
USEUNIT("IniOpts.pas");
USEUNIT("IniStrings.pas");
USEUNIT("Main.pas");
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
        try
        {
                 Application->Initialize();
                 Application->Run();
        }
        catch (Exception &exception)
        {
                 Application->ShowException(&exception);
        }
        return 0;
}
//---------------------------------------------------------------------------

