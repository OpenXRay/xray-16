//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("FONT.RES");
USERES("KILLDATE.RES");
USERES("TrayDays.res");
USEUNIT("CalOptions.pas");
USEFORMNS("frmCalConfig.pas", Frmcalconfig, CalConfigForm);
USEFORMNS("frmCalend.pas", Frmcalend, CalendarForm);
USEFORMNS("frmDateProp.pas", Frmdateprop, DatePropForm);
USEFORMNS("frmHolidayProp.pas", Frmholidayprop, HolidayPropForm);
USEUNIT("Fake.pas");
//---------------------------------------------------------------------------
#include <fake.hpp>
//---------------------------------------------------------------------------

WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Application->Initialize();
    Application->Title = "EldoS TrayDays";
    FakeClass = new TFakeClass();
    try
    {
      FakeClass->Run();
    }
     __finally
    {
      FakeClass->Free();
    }
    return 0;
}
//---------------------------------------------------------------------------
