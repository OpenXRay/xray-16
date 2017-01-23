/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include <stdlib.h>
#include "gt2aMain.h"
#include "gt2aInput.h"
#include "gt2aClient.h"
#include "gt2aDisplay.h"
#include "gt2aSound.h"

#define TOGGLE(b)  { ((b) = !(b)); printf( #b " = %s\n", (b)?"ON":"OFF"); }

GT2Bool upPressed;
GT2Bool downPressed;
GT2Bool leftPressed;
GT2Bool rightPressed;

GT2Bool UseJoystick = GT2False;

void SetKeyboardNormal(void);
void SetKeyboardChat(void);

static char chatBuffer[CHAT_MAX];
static int nChatChars;
static GT2Bool chatting;

static void KeyboardPress
(
	unsigned char key,
	int x,
	int y
)
{
	// Lowercase it.
	////////////////
	key = tolower(key);

	switch(key)
	{
	case 0x1B: // esc
		ShutdownDisplay();
		exit(1);

	case 't':
		SetKeyboardChat();
		break;

	case 'p':
		TOGGLE(DrawWithPredictionOption);
		break;

	case 'c':
		TOGGLE(ViewClippingOption);
		break;

	case 'w':
		if(localIndex != -1)
		{
			char buffer[32];
			Player * player;

			player = &players[localIndex];
			sprintf(buffer, "%.0f %.0f %.0f", player->position[0], player->position[1], player->rotation);
			DisplayChat(buffer);
		}
		break;

	case 'j':
		TOGGLE(UseJoystick);
		break;

	case 'g':
		TOGGLE(DrawGraphsOption);
		break;

	case 'r':
		TOGGLE(DrawBoundingCirclesOption);
		break;
	}
}

static void KeyboardRelease
(
	unsigned char key,
	int x,
	int y
)
{
	// Lowercase it.
	////////////////
	key = tolower(key);

	switch(key)
	{
	case 'b':
		SendPress("mine");
		break;

	case ' ':
		SendPress("rocket");
		break;
	}
}

static void SpecialKeyboardPress
(
	int key,
	int dummyx,
	int dummyy
)
{
	switch(key)
	{
	case GLUT_KEY_UP:
		upPressed = GT2True;
		localMotion = FORWARD;
		break;

	case GLUT_KEY_DOWN:
		downPressed = GT2True;
		localMotion = BACKWARD;
		break;

	case GLUT_KEY_LEFT:
		leftPressed = GT2True;
		localTurning = LEFT;
		break;

	case GLUT_KEY_RIGHT:
		rightPressed = GT2True;
		localTurning = RIGHT;
		break;
	}
}

static void SpecialKeyboardRelease
(
	int key,
	int dummyx,
	int dummyy
)
{
	switch(key)
	{
	case GLUT_KEY_UP:
		upPressed = GT2False;
		if(downPressed)
			localMotion = BACKWARD;
		else
			localMotion = STILL;
		break;

	case GLUT_KEY_DOWN:
		downPressed = GT2False;
		if(upPressed)
			localMotion = FORWARD;
		else
			localMotion = STILL;
		break;

	case GLUT_KEY_LEFT:
		leftPressed = GT2False;
		if(rightPressed)
			localTurning = RIGHT;
		else
			localTurning = STILL;
		break;

	case GLUT_KEY_RIGHT:
		rightPressed = GT2False;
		if(leftPressed)
			localTurning = LEFT;
		else
			localTurning = STILL;
		break;
	}
}

const char * GetChatBuffer
(
	void
)
{
	if(chatting)
		return (const char *)chatBuffer;
	
	return NULL;
}

static void ChatAddChar
(
	char c
)
{
	if(nChatChars < (CHAT_MAX - 1))
	{
		chatBuffer[nChatChars++] = c;
		chatBuffer[nChatChars] = '\0';
	}
}

static void ChatBackspace
(
	void
)
{
	if(nChatChars)
		chatBuffer[--nChatChars] = '\0';
}

static void ChatClear
(
	void
)
{
	chatBuffer[0] = '\0';
	nChatChars = 0;
}

static void ChatSend
(
	void
)
{
	assert(nChatChars < CHAT_MAX);

	// Send it.
	///////////
	if(nChatChars > 0)
		SendChat(chatBuffer);
}

static void ChatKeyboardPress
(
	unsigned char key,
	int x,
	int y
)
{
	switch(key)
	{
	case 0x1B: // esc
		// Back to normal keyboard handling.
		////////////////////////////////////
		SetKeyboardNormal();

		return;

	case 0x0D: // enter
		// Send the current chat message.
		/////////////////////////////////
		ChatSend();

		// Back to normal keyboard handling.
		////////////////////////////////////
		SetKeyboardNormal();

		return;

	case 0x08: // backspace
	case 0x7F: // delete
		ChatBackspace();
		break;
	}

	// Is it printable?
	///////////////////
	if(isprint(key))
	{
		// Add it to the buffer.
		////////////////////////
		ChatAddChar(key);
	}
}

static void ChatKeyboardRelease
(
	unsigned char key,
	int x,
	int y
)
{
	switch(key)
	{
	}
}

void SetKeyboardNormal
(
	void
)
{
	// Ignore key repeats.
	//////////////////////
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	// Set glut callbacks.
	//////////////////////
	glutKeyboardFunc(KeyboardPress);
	glutKeyboardUpFunc(KeyboardRelease);

	// We're not chatting.
	//////////////////////
	chatting = GT2False;
}

void SetKeyboardChat
(
	void
)
{
	// We don't care about key repeats now.
	///////////////////////////////////////
	glutSetKeyRepeat(GLUT_KEY_REPEAT_DEFAULT);

	// Set glut callbacks.
	//////////////////////
	glutKeyboardFunc(ChatKeyboardPress);
	glutKeyboardUpFunc(ChatKeyboardRelease);

	// Clear the chat buffer.
	/////////////////////////
	ChatClear();

	// We're chatting.
	//////////////////
	chatting = GT2True;
}
static void MouseMotionPassive(int x, int y)
{
	static int x_hold = 0;
	static int y_hold = 0;

	if(x_hold)
	{
		localRotation += -(x_hold - x);

	}

	if( (x <= 0) || (x >= 639) )
		glutWarpPointer(320,240);

	x_hold = x;
	y_hold = y;
}

static void MouseButton(int button, int state, int x, int y)
{
	switch(button)
	{
	case GLUT_LEFT_BUTTON:
		if(state == GLUT_DOWN)
			SendPress("rocket");
		break;
	case GLUT_RIGHT_BUTTON:
		switch(state)
		{
		case GLUT_DOWN:
			upPressed = GT2True;
			localMotion = FORWARD;
			break;
		case GLUT_UP:
			upPressed = GT2False;
			localMotion = STILL;
			break;
		}
		break;
	case GLUT_MIDDLE_BUTTON:
		switch(state)
		{
		case GLUT_DOWN:
			upPressed = GT2True;
			localMotion = BACKWARD;
			break;
		case GLUT_UP:
			upPressed = GT2False;
			localMotion = STILL;
			break;
		}
		break;
	}
}


static void JoystickProcess(unsigned int buttonMask, int x, int y, int z)
{
	static BYTE buttonA = GT2False;


	if(!UseJoystick)
		return;

	if(buttonMask & GLUT_JOYSTICK_BUTTON_A)
	{
		if(buttonA == GT2False)
		{
			SendPress("rocket");
			buttonA = GT2True;
		}
	}
	else
		buttonA = GT2False;

	if (x < -50)
		localTurning = LEFT;
	else if(x > 50)
		localTurning = RIGHT;
	else
		localTurning = STILL;

	if(z < -300)
		localMotion = FORWARD;
	else if(z > 300)
		localMotion = BACKWARD;
	else
		localMotion = STILL;
		
}

static int MainMenu;
enum
{
	MainToggleSound,
	MainToggleViewClipping
};
static void MainMenuCallback
(
	int item
)
{
	switch(item)
	{
	case MainToggleSound:
		ToggleSound();
		break;
	case MainToggleViewClipping:
		TOGGLE(ViewClippingOption);
		break;
	}
}

static int GraphMenu;
enum
{
	GraphToggleGraphs,
	GraphToggleUpdateTime,
	GraphToggleUpdateLength,
	GraphToggleFrameTime,
	GraphToggleMarks
};
static void GraphMenuCallback
(
	int item
)
{
	switch(item)
	{
	case GraphToggleGraphs:
		TOGGLE(DrawGraphsOption);
		break;
	case GraphToggleUpdateTime:
		TOGGLE(DrawUpdateTimeOption);
		break;
	case GraphToggleUpdateLength:
		TOGGLE(DrawUpdateLengthOption);
		break;
	case GraphToggleFrameTime:
		TOGGLE(DrawFrameTimeOption);
		break;
	case GraphToggleMarks:
		TOGGLE(DrawMarksOption);
		break;
	}
}

static int DrawMenu;
enum
{
	DrawToggleFPS,
	DrawTogglePrediction,
	DrawToggleBackground,
	DrawToggleStars,
	DrawToggleBoundingCircles
};
static void DrawMenuCallback
(
	int item
)
{
	switch(item)
	{
	case DrawToggleFPS:
		TOGGLE(DrawFPSOption);
		break;
	case DrawTogglePrediction:
		TOGGLE(DrawWithPredictionOption);
		break;
	case DrawToggleBackground:
		TOGGLE(DrawBackgroundOption);
		break;
	case DrawToggleStars:
		TOGGLE(DrawStarsOption);
		break;
	case DrawToggleBoundingCircles:
		TOGGLE(DrawBoundingCirclesOption);
		break;
	}
}

static int RadarMenu;
enum
{
	RadarDrawRadar,
	RadarSmallRadar,
	RadarMediumRadar,
	RadarLargeRadar,
	RadarSmallPoints,
	RadarMediumPoints,
	RadarLargePoints
};
static void RadarMenuCallback
(
	int item
)
{
	switch(item)
	{
	case RadarDrawRadar:
		TOGGLE(DrawRadarOption);
		break;
	case RadarSmallRadar:
		RadarScaleOption = 0.5;
		break;
	case RadarMediumRadar:
		RadarScaleOption = 1;
		break;
	case RadarLargeRadar:
		RadarScaleOption = 2;
		break;
	case RadarSmallPoints:
		RadarPointScaleOption = 1;
		break;
	case RadarMediumPoints:
		RadarPointScaleOption = 2;
		break;
	case RadarLargePoints:
		RadarPointScaleOption = 3;
		break;
	}
}

void InitializeInput
(
	void
)
{
	// Set the keyboard to normal.
	//////////////////////////////
	SetKeyboardNormal();

	// The glut handlers for "special" keys don't change.
	/////////////////////////////////////////////////////
	glutSpecialFunc(SpecialKeyboardPress);
	glutSpecialUpFunc(SpecialKeyboardRelease);
	

	// The glut handlers for mouse input
	////////////////////////////////////
	glutPassiveMotionFunc(MouseMotionPassive);
	glutMotionFunc(MouseMotionPassive);
	glutMouseFunc (MouseButton);

	// The glut handler for joystick input
	//////////////////////////////////////
	glutJoystickFunc(JoystickProcess, 1);

	// Setup the menu.
	//////////////////
	MainMenu = glutCreateMenu(MainMenuCallback);

	// Graph sub-menu.
	//////////////////
	GraphMenu = glutCreateMenu(GraphMenuCallback);
	glutSetMenu(GraphMenu);
	glutAddMenuEntry("Toggle Graphs", GraphToggleGraphs);
	glutAddMenuEntry("Toggle Update Time", GraphToggleUpdateTime);
	glutAddMenuEntry("Toggle Update Length", GraphToggleUpdateLength);
	glutAddMenuEntry("Toggle Frame Time", GraphToggleFrameTime);
	glutAddMenuEntry("Toggle Marks", GraphToggleMarks);
	glutSetMenu(MainMenu);
	glutAddSubMenu("Graph", GraphMenu);

	// Draw sub-menu.
	/////////////////
	DrawMenu = glutCreateMenu(DrawMenuCallback);
	glutSetMenu(DrawMenu);
	glutAddMenuEntry("Toggle FPS", DrawToggleFPS);
	glutAddMenuEntry("Toggle Prediction", DrawTogglePrediction);
	glutAddMenuEntry("Toggle Background", DrawToggleBackground);
	glutAddMenuEntry("Toggle Stars", DrawToggleStars);
	glutAddMenuEntry("Toggle Bounding Circles", DrawToggleBoundingCircles);
	glutSetMenu(MainMenu);
	glutAddSubMenu("Draw", DrawMenu);

	// Radar sub-menu.
	//////////////////
	RadarMenu = glutCreateMenu(RadarMenuCallback);
	glutSetMenu(RadarMenu);
	glutAddMenuEntry("Draw Radar", RadarDrawRadar);
	glutAddMenuEntry("Small Radar", RadarSmallRadar);
	glutAddMenuEntry("Medium Radar", RadarMediumRadar);
	glutAddMenuEntry("Large Radar", RadarLargeRadar);
	glutAddMenuEntry("Small Points", RadarSmallPoints);
	glutAddMenuEntry("Medium Points", RadarMediumPoints);
	glutAddMenuEntry("Large Points", RadarLargePoints);
	glutSetMenu(MainMenu);
	glutAddSubMenu("Radar", RadarMenu);

	// Main menu.
	/////////////
	glutAddMenuEntry("Toggle Sound", MainToggleSound);
	glutAddMenuEntry("Toggle View Clipping", MainToggleViewClipping);

	// Attach the menu.
	///////////////////
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}
