//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop
/*
#include <float.h>
#include <math.h>
#include <xrCore.h>
#include <_types.h>
*/
#include <stdio.h>

#define cosf cos
#define fabsf fabs
#define sinf sin
#define sqrtf sqrt
#define EDevice NULL
#define Device NULL

#pragma comment (lib, "x:\xrCoreB.lib")
//---------------------------------------------------------------------------
USEFORM("main.cpp", MainForm);
USEFORM("float_param.cpp", frmTimeConstructor);
USEFORM("float_constructor.cpp", frmConstructor);
USEFORM("color.cpp", AddColorForm);
USEFORM("single_param.cpp", AddFloatForm);
//---------------------------------------------------------------------------
FILE                        *g_LogFileHandle            =       NULL;
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void    log_callback    (LPCSTR lpString)
{
    if (!g_LogFileHandle)
       return;
    fprintf (g_LogFileHandle, "%s\n", lpString);
}


WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
        g_LogFileHandle = fopen ("postprocess.log", "wt");
         Core._initialize ("Postprocess editor", log_callback, FALSE);
         FS._initialize (CLocatorAPI::flScanAppRoot, 0, 0);
         
         Application->Initialize();
         Application->CreateForm(__classid(TMainForm), &MainForm);
		Application->Run();
         Core._destroy();
         fclose (g_LogFileHandle);
    }
    catch (Exception &exception)
    {
        fclose (g_LogFileHandle);
         Application->ShowException(&exception);
    }
    catch (...)
    {
         try
         {
             throw Exception("");
         }
         catch (Exception &exception)
         {
            fclose (g_LogFileHandle);
             Application->ShowException(&exception);
         }
    }
    return 0;
}
//---------------------------------------------------------------------------
