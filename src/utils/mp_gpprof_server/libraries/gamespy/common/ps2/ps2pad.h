#ifndef __PS2PAD_H__
#define __PS2PAD_H__

#include "../../../nonport.h"

typedef enum
{
	PadLeft,
	PadDown,
	PadRight,
	PadUp,
	PadStart,
	PadRightStick,
	PadLeftStick,
	PadSelect,
	PadSquare,
	PadX,
	PadCircle,
	PadTriangle,
	PadR1,
	PadL1,
	PadR2,
	PadL2,
	NumPadEvents
} PadEvents;

int PadInit(void);
void PadReadInput(int events[NumPadEvents]);

#endif
