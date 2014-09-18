//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ColorPicker.h"

class CTCD{
public:
	TColorDialog* cdColor;
public:
	CTCD(){
		cdColor = xr_new<TColorDialog>((TComponent*)0);
        cdColor->Options = TColorDialogOptions()<<cdFullOpen;
    }
    ~CTCD(){
    	xr_delete(cdColor);
    }
};
static CTCD TCD;

extern "C" DLL_API bool FSColorPickerExecute(u32* currentColor, LPDWORD originalColor, const int initialExpansionState);
bool SelectColor(u32* currentcolor, bool bDefaultPicker){
	VERIFY(currentcolor);
	if (bDefaultPicker){
        TCD.cdColor->Color = TColor(rgb2bgr(*currentcolor));
        if (TCD.cdColor->Execute()){
			*currentcolor = bgr2rgb(TCD.cdColor->Color);
        	return true;
        }
        return false;
    }else{
    	u32 clr=*currentcolor;
  	    if (FSColorPickerExecute(&clr, 0, 0)){
        	*currentcolor = clr;
         	return true;
        }
        return false;
    }
}

bool SelectColorWin(u32* currentcolor, bool bDefaultPicker){
	VERIFY(currentcolor);
	if (bDefaultPicker){
        TCD.cdColor->Color = TColor(*currentcolor);
        if (TCD.cdColor->Execute()){
			*currentcolor = TCD.cdColor->Color;
        	return true;
        }
        return false;
    }else{
        u32 cur = bgr2rgb(*currentcolor);
        if (FSColorPickerExecute(&cur, 0, 0)){
			*currentcolor = rgb2bgr(cur);
        	return true;
        }
	    return false;
    }
}

