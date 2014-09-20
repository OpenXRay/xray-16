#include "ps2pad.h"
#include <libpad.h>
#include <libdma.h>
#include <libgraph.h>

#define BUTTON_LEFT(d)		((d[2] & (1 << (3+4)))==0)
#define BUTTON_DOWN(d)		((d[2] & (1 << (2+4)))==0)
#define BUTTON_RIGHT(d)		((d[2] & (1 << (1+4)))==0)
#define BUTTON_UP(d)		((d[2] & (1 << (0+4)))==0)

#define BUTTON_START(d)		((d[2] & (1 << 3))==0)
#define BUTTON_RSTICK(d)	((d[2] & (1 << 2))==0)
#define BUTTON_LSTICK(d)	((d[2] & (1 << 1))==0)
#define BUTTON_SELECT(d)	((d[2] & (1 << 0))==0)

#define BUTTON_SQUARE(d)	((d[3] & (1 << (3+4)))==0)
#define BUTTON_X(d)			((d[3] & (1 << (2+4)))==0)
#define BUTTON_CIRCLE(d)	((d[3] & (1 << (1+4)))==0)
#define BUTTON_TRI(d)		((d[3] & (1 << (0+4)))==0)

#define BUTTON_R1(d)		((d[3] & (1 << 3))==0)
#define BUTTON_L1(d)		((d[3] & (1 << 2))==0)
#define BUTTON_R2(d)		((d[3] & (1 << 1))==0)
#define BUTTON_L2(d)		((d[3] & (1 << 0))==0)

static int term_id = 0;		

static u_long128 mPadDMABuf[scePadDmaBufferMax] __attribute__((aligned (64)));
static int mPadState;
static int mPadPhase;

static unsigned char mActDirect[6];
static unsigned char mActAlign[6];

static unsigned short PadReadData(int events[NumPadEvents])
{
	static unsigned char rdata_prev[32];
	unsigned char rdata[32];
	unsigned short tpad = 0;
	unsigned short paddata = 0;
	static unsigned short rpad = 0;

	if (scePadRead(0,0,rdata) == 0)
		return 0;

	if (rdata[0] == 0)
	{
		paddata = (unsigned short)(0xffff ^ ((rdata[2]<<8)|rdata[3]));

		tpad = (unsigned short)(paddata & ~rpad);
		rpad = paddata;

		term_id = (rdata[1]>>4);

		events[PadLeft] = (BUTTON_LEFT(rdata) && !BUTTON_LEFT(rdata_prev));
		events[PadDown] = (BUTTON_DOWN(rdata) && !BUTTON_DOWN(rdata_prev));
		events[PadRight] = (BUTTON_RIGHT(rdata) && !BUTTON_RIGHT(rdata_prev));
		events[PadUp] = (BUTTON_UP(rdata) && !BUTTON_UP(rdata_prev));
		events[PadStart] = (BUTTON_START(rdata) && !BUTTON_START(rdata_prev));
		events[PadRightStick] = (BUTTON_RSTICK(rdata) && !BUTTON_RSTICK(rdata_prev));
		events[PadLeftStick] = (BUTTON_LSTICK(rdata) && !BUTTON_LSTICK(rdata_prev));
		events[PadSelect] = (BUTTON_SELECT(rdata) && !BUTTON_SELECT(rdata_prev));
		events[PadSquare] = (BUTTON_SQUARE(rdata) && !BUTTON_SQUARE(rdata_prev));
		events[PadX] = (BUTTON_X(rdata) && !BUTTON_X(rdata_prev));
		events[PadCircle] = (BUTTON_CIRCLE(rdata) && !BUTTON_CIRCLE(rdata_prev));
		events[PadTriangle] = (BUTTON_TRI(rdata) && !BUTTON_TRI(rdata_prev));
		events[PadR1] = (BUTTON_R1(rdata) && !BUTTON_R1(rdata_prev));
		events[PadL1] = (BUTTON_L1(rdata) && !BUTTON_L1(rdata_prev));
		events[PadR2] = (BUTTON_R2(rdata) && !BUTTON_R2(rdata_prev));
		events[PadL2] = (BUTTON_L2(rdata) && !BUTTON_L2(rdata_prev));
	}

	// store current data
	memcpy(rdata_prev, rdata, 32);

	return (tpad);
}

#define ROOT_DIR "host0:/usr/local/sce/iop/modules/"
int PadInit(void)
{
	int i;

	mPadPhase = 0;
	mPadState = 0;

	for (i=0; i <6; i++)
	{
		mActDirect[i] = 0;
		mActAlign[i] = 0;
	}
	
	// Load serial io module
	if (0 >= sceSifLoadModule(ROOT_DIR "sio2man.irx", 0, NULL))
		return 0;

	// Load control pad module
	if (0 >= sceSifLoadModule(ROOT_DIR "padman.irx", 0, NULL))
		return 0;

	// Misc init
	sceDmaReset(1);			// reset DMA
	sceGsResetPath();		// reset GS
	sceGsSyncPath(0, 0);	// wait for completion

	// Open the pad port
	scePadInit(0);
	if (0 == scePadPortOpen(0, 0, mPadDMABuf))
		return 0; // couldn't initialize pad

	return 1;
}

void PadReadInput(int events[NumPadEvents])
{
	static int id = 0;
	int exid;
	int i;

	for(i = 0 ; i < NumPadEvents ; i++)
		events[i] = 0;

	// check buttons or some junk
	mPadState = scePadGetState(0, 0);
	//if (mPadState >= 0 && state <= 7)
	//	scePadStateIntToStr(state, buf); // get error string
	if (mPadState == scePadStateDiscon)
		mPadPhase = 0; // lost pad

	switch(mPadPhase)
	{
	case 0:
		{
			// Wait until stable or need to find ctp1
			if (mPadState != scePadStateStable && mPadState != scePadStateFindCTP1)
				break;

			// Get controller ID
			id = scePadInfoMode(0, 0, InfoModeCurID, 0);
			if (id == 0)
				break;

			// Is there an extended ID?
			exid = scePadInfoMode(0,0, InfoModeCurExID, 0);
			if (exid > 0)
				id = exid;

			if (id == 40)
				// Special processing for "standard" controller
				mPadPhase = 40;
			else if (id == 7)
				// Special processing for "analog" controller
				mPadPhase = 70;
			else
				// Skip to end of setup
				mPadPhase = 99;
			break;
		}

	// 1st step special processing for "standard" controller
	case 40: 
		if (scePadInfoMode(0, 0, InfoModeCurExID, 0)==0)
		{
			// Skip to end if this was set from an extended ID
			mPadPhase = 99;
			break;
		}

		// Set main mode
		if (scePadSetMainMode(0, 0, 1, 0)==1)
			mPadPhase++;
		break;

	// 2nd step special processing for "standard" controller
	case 41:
		if (scePadGetReqState(0,0) == scePadReqStateFaild)
			mPadPhase--; // failed, go back a phase
		if (scePadGetReqState(0,0) == scePadReqStateComplete)
			mPadPhase = 0; // completed, go back to beginning to try again
		break;

	// 1st step special processing for "analog" controller
	case 70:
		if (scePadInfoAct(0, 0, -1, 0)==0)
			mPadPhase = 99; // done
		mActAlign[0] = 0;
		mActAlign[1] = 1;
		for (i=2; i<6; i++)
			mActAlign[i] = 0xff;
		if (scePadSetActAlign(0, 0, mActAlign))
			mPadPhase++;
		break;

	// 2nd step special processing for "analog" controller
	case 71:
		if (scePadGetReqState(0,0) == scePadReqStateFaild)
			mPadPhase--;
		if (scePadGetReqState(0,0) == scePadReqStateComplete)
			mPadPhase = 99; // finished, jump to end

	case 99:
	default:
		if (mPadState == scePadStateStable || mPadState == scePadStateFindCTP1)
		{
			// read button states
			//unsigned short pad = ReadPadData();
			PadReadData(events);

			if (term_id != id)
				mPadPhase = 0;
		}
		break;
	};
}
