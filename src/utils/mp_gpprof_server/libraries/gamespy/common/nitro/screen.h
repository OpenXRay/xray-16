#ifndef _SCREEN_H_
#define _SCREEN_H_

#define SCREEN_WIDTH    32
#define SCREEN_HEIGHT   24

typedef enum
{
	SCBlack,
	SCRed,
	SCGreen,
	SCBlue,
	SCYellow,
	SCPurple,
	SCLightBlue,
	SCDarkRed,
	SCDarkGreen,
	SCDarkBlue,
	SCDarkYellow,
	SCDarkPurple,
	SCDarkLightBlue,
	SCGray,
	SCDarkGray,
	SCWhite
} ScreenColor;

// can be combined
#define PRINT_TO_SCREEN    1  // default
#define PRINT_TO_DEBUGGER  2

void ScreenInit(void);

void ClearTopScreen(void);
void ClearBottomScreen(void);
void ClearScreens(void);

void PrintChar(char c);
void Printf(const char* format, ...);
void VPrintf(const char* format, va_list args);
void SetPrintMode(int mode);

void SetTopScreenLine(int line, ScreenColor color, const char * text);
void SetTopScreenLineCentered(int line, ScreenColor color, const char * text);
void SetTopScreenLineHighlight(int line, ScreenColor color, const char * text, int pos, int range, ScreenColor posColor);

void SetBottomScreenLine(int line, ScreenColor color, const char * text);
void SetBottomScreenLineCentered(int line, ScreenColor color, const char * text);
void SetBottomScreenLineHighlight(int line, ScreenColor color, const char * text, int pos, int range, ScreenColor posColor);

#endif