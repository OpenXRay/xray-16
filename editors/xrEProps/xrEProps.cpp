#include "stdafx.h"
#pragma hdrstop

#include "../../xrServerEntities/xrEProps.h"

USEFORM("ChoseForm.cpp", frmChoseItem);
USEFORM("ItemDialog.cpp", frmItemDialog);
USEFORM("ItemList.cpp", ItemList);
USEFORM("NumericVector.cpp", frmNumericVector);
USEFORM("PropertiesList.cpp", Properties);
USEFORM("ShaderFunction.cpp", frmShaderFunction);
USEFORM("TextForm.cpp", frmText);
//---------------------------------------------------------------------------
#pragma package(smart_init)
void XR_EPROPS_API CheckWindowPos(TForm* form)
{
	if (form->Left+form->Width>Screen->Width) 	form->Left	= Screen->Width-form->Width;
	if (form->Top+form->Height>Screen->Height)	form->Top 	= Screen->Height-form->Height;
	if (form->Left<0) 							form->Left	= 0;
	if (form->Top<0) 							form->Top 	= 0;
}
//---------------------------------------------------------------------------

//#pragma argsused
int WINAPI DllMain(HINSTANCE hinst, unsigned long reason, void*)
{
	return 1;
}
//---------------------------------------------------------------------------
 