#include "../nonport.h"
#include "touch.h"

void TouchInit(void)
{
    TPCalibrateParam calibrate;

    TP_Init();

	if(!TP_GetUserInfo(&calibrate))
		OS_Panic("Failed to initialize touch panel");

	TP_SetCalibrateParam(&calibrate);
}

// 0 <= x <= 255
// 0 <= y <= 191
BOOL GetTouch(int * x, int * y)
{
	TPData data;
	u32 result;

	result = TP_RequestCalibratedSampling(&data);

	if((result != 0) || (data.touch == TP_TOUCH_OFF))
		return FALSE;

	if(x)
		*x = data.x;
	if(y)
		*y = data.y;

	return TRUE;
}

void WaitForTouch(void)
{
	while(1)
	{
		SVC_WaitVBlankIntr();
		if(GetTouch(NULL, NULL))
			break;
	}
}