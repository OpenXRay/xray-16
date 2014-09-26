//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("elpproB6.res");
USEPACKAGE("vclx.bpi");
USEPACKAGE("VCL.bpi");
USEPACKAGE("rtl.bpi");
USEPACKAGE("ElPackB6.bpi");
USEUNIT("Source\ElShellCtl.pas");
USEUNIT("Source\ElPrinter.pas");
USEUNIT("Source\ElTreePrinter.pas");
USEFORMNS("Source\ElPrinterPreview.pas", ElPrinterPreview, ElPrinterPreviewDlg);
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
//   Package source.
//---------------------------------------------------------------------------
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void*)
{
    return 1;
}
//---------------------------------------------------------------------------

