#include "..\nonport.h"
#include "screen.h"
#include "font.h"

static u16 gTopScreen[SCREEN_HEIGHT * SCREEN_WIDTH];
static u16 gBottomScreen[SCREEN_HEIGHT * SCREEN_WIDTH];
static int gPos;
static int gPrintMode = PRINT_TO_DEBUGGER;

static void VBlankIntr(void)
{
	// Reflect virtual screen to VRAM
	DC_FlushRange( gTopScreen , sizeof( gTopScreen ) );
	DC_FlushRange( gBottomScreen , sizeof( gBottomScreen ) );
	GX_LoadBG0Scr( gTopScreen , 0 , sizeof( gTopScreen ) );
	GXS_LoadBG0Scr( gBottomScreen , 0 , sizeof( gBottomScreen ) );

	// Set IRQ check flag
	OS_SetIrqCheckFlag( OS_IE_V_BLANK );
}

void ClearTopScreen(void)
{
    MI_CpuClearFast( (void*)gTopScreen , sizeof( gTopScreen ) );
//	DC_FlushRange( gTopScreen , sizeof( gTopScreen ) );
}

void ClearBottomScreen(void)
{
    MI_CpuClearFast( (void*)gBottomScreen , sizeof( gBottomScreen ) );
//	DC_FlushRange( gBottomScreen , sizeof( gBottomScreen ) );
}

void ClearScreens(void)
{
	ClearTopScreen();
	ClearBottomScreen();
}

static void ScrollTopScreen(void)
{
	int i;
	for(i = 0 ; i < (SCREEN_HEIGHT - 1) ; i++)
		MI_CpuCopyFast((void*)&gTopScreen[SCREEN_WIDTH*(i+1)], (void*)&gTopScreen[SCREEN_WIDTH*i], SCREEN_WIDTH*sizeof(gTopScreen[0]));
	MI_CpuClearFast((void*)&gTopScreen[(SCREEN_HEIGHT-1)*SCREEN_WIDTH], SCREEN_WIDTH*sizeof(gTopScreen[0]));
}

void PrintChar(char c)
{
    u8 palette = 0xf;
    
    if(c == '\r')
    	return;
    
    if(gPos == SCREEN_WIDTH)
    {
    	ScrollTopScreen();
		gPos = 0;
		if(c == '\n')
			return;
	}
	
	if(c == '\n')
	{
		gPos = SCREEN_WIDTH;
	}
	else
	{
		gTopScreen[((SCREEN_HEIGHT - 1) * SCREEN_WIDTH) + gPos] = (u16)((palette << 12) | c);
		gPos++;
	}
}

void Printf(const char* format, ...)
{
	va_list vlist;

	va_start(vlist, format);
	VPrintf(format, vlist);
	va_end(vlist);
}

void VPrintf(const char* format, va_list args)
{
	if(gPrintMode & PRINT_TO_SCREEN)
	{
		static char text[2048];
		int i;

		vsnprintf(text, sizeof(text) - 1, format, args);
		text[sizeof(text) - 1] = '\0';

		for(i = 0 ; text[i] ; i++)
			PrintChar(text[i]);

		SVC_WaitVBlankIntr();
	}
	if(gPrintMode & PRINT_TO_DEBUGGER)
	{
		OS_VPrintf(format, args);
	}
}

void SetPrintMode(int mode)
{
	gPrintMode = mode;
}

static void SetScreenLine(u16 * screen, int line, int offset, ScreenColor color, const char * text, int pos, int range, ScreenColor posColor)
{
	u8 palette = (u8)color;
	u8 posPalette = (u8)posColor;
	u16 val;
	int i;
	char c;
	BOOL inRange;

    MI_CpuClearFast((void*)&screen[line*SCREEN_WIDTH], SCREEN_WIDTH*sizeof(gTopScreen[0]));

	for(i = 0 ; i < SCREEN_WIDTH ; i++)
	{
		c = text[i];

		if(c == '\0')
			break;

		inRange = ((i >= pos) && (i < (pos + range)))?TRUE:FALSE;

		val = (u16)(((inRange?posPalette:palette) << 12) | c);
		screen[(line * SCREEN_WIDTH) + offset + i] = val;
	}
}

static void SetScreenLineCentered(u16 * screen, int line, ScreenColor color, const char * text)
{
	int len;
	int offset;

	len = (int)strlen(text);
	if(len < SCREEN_WIDTH)
		offset = (((SCREEN_WIDTH - len) & ~1) / 2);
	else
		offset = 0;

	SetScreenLine(screen, line, offset, color, text, 0, 0, color);
}

void SetTopScreenLine(int line, ScreenColor color, const char * text)
{
	SetScreenLine(gTopScreen, line, 0, color, text, 0, 0, color);
}

void SetTopScreenLineCentered(int line, ScreenColor color, const char * text)
{
	SetScreenLineCentered(gTopScreen, line, color, text);
}

void SetTopScreenLineHighlight(int line, ScreenColor color, const char * text, int pos, int range, ScreenColor posColor)
{
	SetScreenLine(gTopScreen, line, 0, color, text, pos, range, posColor);
}

void SetBottomScreenLine(int line, ScreenColor color, const char * text)
{
	SetScreenLine(gBottomScreen, line, 0, color, text, 0, 0, color);
}

void SetBottomScreenLineCentered(int line, ScreenColor color, const char * text)
{
	SetScreenLineCentered(gBottomScreen, line, color, text);
}

void SetBottomScreenLineHighlight(int line, ScreenColor color, const char * text, int pos, int range, ScreenColor posColor)
{
	SetScreenLine(gBottomScreen, line, 0, color, text, pos, range, posColor);
}

void ScreenInit(void)
{
	// init the graphics engine
	GX_Init();

	// turn off the display engine output
	GX_DispOff();
	GXS_DispOff();

	// setup the display memory
	GX_SetBankForLCDC( GX_VRAM_LCDC_ALL );
	MI_CpuClearFast( (void*)HW_LCDC_VRAM , HW_LCDC_VRAM_SIZE );
	GX_DisableBankForLCDC();
	MI_CpuFillFast( (void*)HW_OAM,192 , HW_OAM_SIZE );
	MI_CpuClearFast( (void*)HW_PLTT , HW_PLTT_SIZE );
	MI_CpuFillFast( (void*)HW_DB_OAM , 192,HW_DB_OAM_SIZE );
	MI_CpuClearFast( (void*)HW_DB_PLTT , HW_DB_PLTT_SIZE );

	// clear the screens
	ClearScreens();

	// 2D display setup for displaying character string
	//g2
	GX_SetBankForBG( GX_VRAM_BG_128_A );
	G2_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256 ,
	                GX_BG_COLORMODE_16 ,
	                GX_BG_SCRBASE_0xf800 ,      // SCR base block 31
	                GX_BG_CHARBASE_0x00000 ,    // CHR base block 0
	                GX_BG_EXTPLTT_01 );
	G2_SetBG0Priority( 0 );
	G2_BG0Mosaic( FALSE );
	GX_SetGraphicsMode( GX_DISPMODE_GRAPHICS , GX_BGMODE_0,GX_BG0_AS_2D );
	GX_SetVisiblePlane( GX_PLANEMASK_BG0 );
	GX_LoadBG0Char( d_CharData , 0 , sizeof( d_CharData ) );
	GX_LoadBGPltt( d_PaletteData , 0 , sizeof( d_PaletteData ) );
	GX_LoadBG0Scr( gTopScreen , 0 , sizeof( gTopScreen ) );
	//g2s
	GX_SetBankForSubBG( GX_VRAM_SUB_BG_128_C );
	G2S_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256 ,
	                GX_BG_COLORMODE_16 ,
	                GX_BG_SCRBASE_0xf800 ,      // SCR base block 31
	                GX_BG_CHARBASE_0x00000 ,    // CHR base block 0
	                GX_BG_EXTPLTT_01 );
	G2S_SetBG0Priority( 0 );
	G2S_BG0Mosaic( FALSE );
	GXS_SetGraphicsMode( GX_BGMODE_0 );
	GXS_SetVisiblePlane( GX_PLANEMASK_BG0 );
	GXS_LoadBG0Char( d_CharData , 0 , sizeof( d_CharData ) );
	GXS_LoadBGPltt( d_PaletteData , 0 , sizeof( d_PaletteData ) );
	GXS_LoadBG0Scr( gBottomScreen , 0 , sizeof( gBottomScreen ) );

	// Interrupt setup
	OS_SetIrqFunction( OS_IE_V_BLANK , VBlankIntr );
	OS_EnableIrqMask( OS_IE_V_BLANK );
	GX_VBlankIntr( TRUE );

	// Start LCD display
	GX_DispOn();
	GXS_DispOn();
}