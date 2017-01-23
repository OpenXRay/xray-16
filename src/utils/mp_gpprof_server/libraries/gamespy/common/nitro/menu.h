#ifndef _MENU_H_
#define _MENU_H_

// various length defines
#define MAX_CHOICE_STRING_LEN      32
#define MAX_TOP_SCREEN_TEXT        32
#define MAX_CHOICES                10
#define MAX_LIST_STRINGS           16
#define MAX_LIST_STRING_LEN        32
#define MAX_KEYBOARD_TEXT_LEN      32
#define MAX_EXTRA_TEXT_STRINGS     10
#define MAX_EXTRA_TEXT_STRING_LEN  32

// options for screens
#define SCREEN_OPTION_ANIMATED         0x1
#define SCREEN_OPTION_KEYBOARD         0x2
#define SCREEN_OPTION_LIST             0x4
#define SCREEN_OPTION_EXTRAS_CENTERED  0x8

// options for screen choices
#define CHOICE_OPTION_NEEDS_LIST_SELECTION   0x1
#define CHOICE_OPTION_DISABLED               0x2

// a single choice on a screen
typedef struct MenuScreenChoice
{
	const char * text;
	int options;
} MenuScreenChoice;

// an instance of a screen
typedef struct MenuScreen
{
	struct MenuScreenConfiguration * configuration;
	char extraText[MAX_EXTRA_TEXT_STRINGS][MAX_EXTRA_TEXT_STRING_LEN + 1];
	char list[MAX_LIST_STRINGS][MAX_LIST_STRING_LEN + 1];
	int listSelection;  // default -1, no selection
	char keyboardText[MAX_KEYBOARD_TEXT_LEN + 1];
	int numChoices;
} MenuScreen;

// the static configuration for a screen
typedef struct MenuScreenConfiguration
{
	const char * topScreenText;
	MenuScreenChoice choices[MAX_CHOICES];
	void (* initFunc)(void);
	void (* choseFunc)(const char * choice);
	void (* thinkFunc)(void);
	int options;
} MenuScreenConfiguration;

// call this to start showing a menu, pass in the initial screen to show
void StartMenuScreen(MenuScreenConfiguration * configuration);

// call this from any of the configuration callback funcs to set the next screen to show
void SetNextMenuScreen(MenuScreenConfiguration * configuration);

// call this to exit the menu
void ExitMenu(void);

// gets the current menu screen
MenuScreen * GetMenuScreen(void);

#endif