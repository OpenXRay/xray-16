//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("dclelpB6.res");
USEUNIT("Design\ElTBDsgn.pas");
USEUNIT("Design\ElEBDsgn.pas");
USEUNIT("Design\TreeDsgn.pas");
USEUNIT("Design\PgCtlProp.pas");
USEFORMNS("Design\MlCapProp.pas", Mlcapprop, MlCapEditDialog);
USEFORMNS("Design\frmFormPers.pas", Frmformpers, TPersPropsForm);
USEFORMNS("Design\frmItemCol.pas", Frmitemcol, ItemColDlg);
USEFORMNS("Design\frmItemsProp.pas", Frmitemsprop, TItemsPropDlg);
USEFORMNS("Design\frmSectEdit.pas", Frmsectedit, SectEdit);
USEFORMNS("Design\frmSectProp.pas", Frmsectprop, ElSectionsPropDlg);
USEFORMNS("Design\frmSoundMap.pas", Frmsoundmap, TSoundMapForm);
USEFORMNS("Design\frmStrPoolEdit.pas", Frmstrpooledit, TStrPoolEditForm);
USEFORMNS("Design\ElMenuDsgn.pas", ElMenuDsgn, TElDesignMenu);
USEUNIT("Design\ColorMapProp.pas");
USEUNIT("Design\ElImageIndexProp.pas");
USEUNIT("ElReg.pas");
USEPACKAGE("vclx.bpi");
USEPACKAGE("VCL.bpi");
USEPACKAGE("elpackB6.bpi");
USEPACKAGE("rtl.bpi");
USEPACKAGE("designide.bpi");
USEPACKAGE("dclstd.bpi");
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

