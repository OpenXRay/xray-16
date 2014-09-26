#include "../nonport.h"
#include "menu.h"
#include "screen.h"
#include "key.h"
#include "touch.h"

static MenuScreen Screen;

static MenuScreenConfiguration * NextMenuScreenConfiguration;

static MenuScreenConfiguration MenuExitScreenConfiguration;

static void NewMenuScreen(MenuScreenConfiguration * configuration)
{
	int count;
	
	memset(&Screen, 0, sizeof(MenuScreen));
	Screen.configuration = configuration;
	Screen.listSelection = -1;
	count = 0;
	while(configuration->choices[count].text && (count < MAX_CHOICES))
		count++;
	Screen.numChoices = count;
}

static void ShowMenuScreen(void)
{
	MenuScreenConfiguration * configuration = Screen.configuration;
	MenuScreenChoice * choices = configuration->choices;
	int options = configuration->options;
	
	const ScreenColor normalColor = SCGray;
	const ScreenColor highlightColor = SCWhite;
	const ScreenColor disabledColor = SCDarkGray;
	const int topTextLine = 10;
	const int animationLine = (topTextLine + 2);
	const int extraTextLine = (animationLine + 2);
	const int keyboardTextLine = 1;
	const int startKeyboardRowsLine = 3;
	const int numKeyboardRows = 3;
	const int numListRows = 5;
	const int startListLine = 1;
	const int startChoicesLine = (SCREEN_HEIGHT - (Screen.numChoices * 2));
	const char keyboardRows[numKeyboardRows][32] =
	{
		"1 2 3 4 5 6 7 8 9 0 - + = _ . < ",
		" A B C D E F G H I J K L M N O  ",
		"  P Q R S T U V W X Y Z  space  "
	};
	const char listBorder[32] = "--------------------";
	const int spacePos = (strchr(keyboardRows[2], 's') - keyboardRows[2]);
	const char animationChars[] = "-*";
	const gsi_time animationTickTime = 500;
	
	BOOL done = FALSE;
	int choiceIndex;
	BOOL keyboard;
	BOOL touching;
	BOOL list;
	BOOL animated;
	int x, y;
	int touchingLine;
	int touchingPos;
	int choiceLine;
	int keyboardLine;
	BOOL wasTouching = FALSE;
	int wasTouchingChoice = -1;
	ScreenColor color;
	int i;
	int range;
	char touchingChar;
	char wasTouchingChar = 0;
	int keyboardPos;
	int numListItems;
	int listScroll = 0;
	BOOL wasTouchingUp = FALSE;
	BOOL wasTouchingDown = FALSE;
	int listItem;
	const char * choiceSelection;
	int animationCount = 0;
	gsi_time lastAnimationTime = current_time();
	gsi_time now;
	char animationText[] = "-";

	// is it animated?
	animated = (options & SCREEN_OPTION_ANIMATED)?TRUE:FALSE;
	
	// is there a keyboard?
	keyboard = (options & SCREEN_OPTION_KEYBOARD)?TRUE:FALSE;

	// is there a list?
	list = (options & SCREEN_OPTION_LIST)?TRUE:FALSE;

	// we can't have a list and a keyboard
	if(keyboard && list)
		OS_Panic("Can't have a keyboard and a list on a menu screen\n");

	// call the init func
	if(configuration->initFunc)
	{
		configuration->initFunc();
		if(NextMenuScreenConfiguration)
			return;
	}

	// if there is a keyboard, get the initial position
	if(keyboard)
		keyboardPos = (int)strlen(Screen.keyboardText);

	// loop until we're done
	while(!done)
	{
		// wait for a screen update to complete
		SVC_WaitVBlankIntr();

		// call the think func
		if(configuration->thinkFunc)
		{
			configuration->thinkFunc();
			if(NextMenuScreenConfiguration)
				return;
		}

		// count the number of list items
		if(list)
		{
			for(numListItems = 0 ; numListItems < MAX_LIST_STRINGS ; numListItems++)
			{
				if(Screen.list[numListItems][0] == '\0')
					break;
			}
		}

		// check for a touch
		touching = GetTouch(&x, &y);
		if(touching)
		{
			// figure out which line we're touching
			touchingLine = (y / (TOUCH_Y_RANGE / SCREEN_HEIGHT));
			
			// figure out which position we're touching
			touchingPos = (x / (TOUCH_X_RANGE / SCREEN_WIDTH));
		}
		else if (wasTouching)
		{
			// check for touching a choice
			if(wasTouchingChoice != -1)
			{
				choiceSelection = choices[wasTouchingChoice].text;
				wasTouchingChoice = -1;

				// call the chose func
				if(configuration->choseFunc)
				{
					configuration->choseFunc(choiceSelection);
					if(NextMenuScreenConfiguration)
						return;
				}
			}
			// check for touching up on the list
			else if(wasTouchingUp)
			{
				if(listScroll > 0)
					listScroll--;
			}
			// check for touching down on the list
			else if(wasTouchingDown)
			{
				if(listScroll < (numListItems - numListRows))
					listScroll++;
			}
			// check for touching a keyboard char
			else if(wasTouchingChar != 0)
			{
				// check for backspace
				if(wasTouchingChar == '<')
				{
					if(keyboardPos > 0)
					{
						keyboardPos--;
						Screen.keyboardText[keyboardPos] = '\0';
					}
				}
				else
				{
					if(keyboardPos < MAX_KEYBOARD_TEXT_LEN)
					{
						Screen.keyboardText[keyboardPos] = wasTouchingChar;
						keyboardPos++;
						Screen.keyboardText[keyboardPos] = '\0';
					}
				}
			}
		}
		wasTouching = touching;
		
		// clear both screens
		ClearScreens();
		
		// show the title on the top screen
		SetTopScreenLineCentered(topTextLine, SCYellow, configuration->topScreenText);

		// show animation
		if(animated)
		{
			now = current_time();
			if((now - lastAnimationTime) >= animationTickTime)
			{
				animationCount++;
				animationCount %= strlen(animationChars);
				lastAnimationTime += animationTickTime;
			}

			animationText[0] = animationChars[animationCount];

			SetTopScreenLineCentered(animationLine, SCYellow, animationText);
		}

		// show extra text
		for(i = 0 ; i < MAX_EXTRA_TEXT_STRINGS ; i++)
		{
			if(options & SCREEN_OPTION_EXTRAS_CENTERED)
				SetTopScreenLineCentered(extraTextLine + i, SCWhite, Screen.extraText[i]);
			else
				SetTopScreenLine(extraTextLine + i, SCWhite, Screen.extraText[i]);
		}
		
		// show the list
		if(list)
		{
			// clear touching vars
			wasTouchingUp = FALSE;
			wasTouchingDown = FALSE;
			
			// show "up" if needed
			if(listScroll > 0)
			{
				if(touching && (touchingLine == startListLine))
				{
					wasTouchingUp = TRUE;
					color = highlightColor;
				}
				else
				{
					color = normalColor;
				}
				SetBottomScreenLineCentered(startListLine, color, "up");
			}
			
			// show the top line
			SetBottomScreenLineCentered(startListLine + 1, normalColor, listBorder);
			
			// show the list items
			for(i = 0 ; i < numListRows ; i++)
			{
				listItem = (listScroll + i);
				
				if(listItem >= numListItems)
					break;
				
				// is this item being touched?
				if(touching && (touchingLine == (startListLine + 2 + i)))
					Screen.listSelection = listItem;
				
				// set the color
				if(Screen.listSelection == listItem)
					color = highlightColor;
				else
					color = normalColor;
				
				// show the item
				SetBottomScreenLineCentered(startListLine + 2 + i, color, Screen.list[listScroll + i]);
			}
			
			// show the bottom line
			SetBottomScreenLineCentered(startListLine + numListRows + 2, normalColor, listBorder);
			
			// show "down" if needed
			if(listScroll < (numListItems - numListRows))
			{
				if(touching && (touchingLine == (startListLine + numListRows + 3)))
				{
					wasTouchingDown = TRUE;
					color = highlightColor;
				}
				else
				{
					color = normalColor;
				}
				SetBottomScreenLineCentered(startListLine + numListRows + 3, color, "down");
			}
		}
		
		// show a keyboard
		if(keyboard)
		{
			// show the text
			SetBottomScreenLine(keyboardTextLine, SCGreen, Screen.keyboardText);
			
			// clear touching var
			wasTouchingChar = 0;
			
			// loop through the keyboard rows
			for(i = 0 ; i < numKeyboardRows ; i++)
			{
				// get the line to show this row on
				keyboardLine = (startKeyboardRowsLine + i);
				
				// check if we're touching this row
				if(touching && (touchingLine == keyboardLine))
				{
					// get the char we're touching
					touchingChar = keyboardRows[i][touchingPos];
					
					// handle touching 'space' specially
					if(islower(touchingChar))
					{
						wasTouchingChar = ' ';
						
						touchingPos = spacePos;
						range = 5;
					}
					else
					{
						if(touchingChar != ' ')
							wasTouchingChar = touchingChar;
						
						range = 1;
					}
					
					// show the keyboard row with the selection highlighted
					SetBottomScreenLineHighlight(
						keyboardLine, normalColor, keyboardRows[i],
						touchingPos, range, highlightColor);
				}
				else
				{
					// show the keyboard row
					SetBottomScreenLine(keyboardLine, normalColor, keyboardRows[i]);
				}
			}
		}
		
		// show the choices
		wasTouchingChoice = -1;
		for(choiceIndex = 0 ; choiceIndex < Screen.numChoices ; choiceIndex++)
		{
			// this is the line to show this choice on
			choiceLine = (startChoicesLine + (choiceIndex * 2));
			
			// check if we're touching this choice
			if((choices[choiceIndex].options & CHOICE_OPTION_DISABLED) || 
				((choices[choiceIndex].options & CHOICE_OPTION_NEEDS_LIST_SELECTION) && (Screen.listSelection == -1)))
			{
				color = disabledColor;
			}
			else if(touching && (touchingLine == choiceLine))
			{
				color = highlightColor;
				wasTouchingChoice = choiceIndex;
			}
			else
			{
				color = normalColor;
			}
			
			// show the line
			SetBottomScreenLineCentered(choiceLine, color, choices[choiceIndex].text);
		}
	}
}

void StartMenuScreen(MenuScreenConfiguration * configuration)
{
	assert(configuration != NULL);

	// set the initial next screen
	SetNextMenuScreen(configuration);

	// loop while there is a next screen to show
	while(NextMenuScreenConfiguration != &MenuExitScreenConfiguration)
	{
		// setup the new screen
		NewMenuScreen(NextMenuScreenConfiguration);

		// clear the next screen setting
		SetNextMenuScreen(NULL);

		// show the menu screen
		ShowMenuScreen();

		// clear the display
		ClearScreens();
	}
}

void SetNextMenuScreen(MenuScreenConfiguration * configuration)
{
	NextMenuScreenConfiguration = configuration;
}

void ExitMenu(void)
{
	SetNextMenuScreen(&MenuExitScreenConfiguration);
}

MenuScreen * GetMenuScreen(void)
{
	return &Screen;
}