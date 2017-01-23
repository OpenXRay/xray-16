/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "gt2aMain.h"
#include "gt2aDisplay.h"
#include "gt2aClient.h"
#include "gt2aServer.h"
#include "gt2aInput.h"
#include "gt2aSound.h"
#include "../../ghttp/ghttp.h"

const V3b Red    = { 255, 0, 0 };
const V3b Green  = { 0, 255, 0 };
const V3b Blue   = { 0, 0, 255 };
const V3b Yellow = { 255, 255, 0 };
const V3b Orange = { 255, 128, 0 };
const V3b Purple = { 255, 0, 255 };
const V3b Black  = { 0, 0, 0 };
const V3b White  = { 255, 255, 255 };
const V3b Grey   = { 128, 128, 128 };

static GT2Bool host = GT2True;
static GT2Bool dedicated;

void Log
(
	const char * format,
	...
)
{
#if 1
	FILE * fp;
	va_list args;

	va_start(args, format);

	fp = fopen("gt2a.log", "at");
	if(fp)
	{
		fprintf(fp, "%08d: ", (current_time() % 100000000));
		vfprintf(fp, format, args);
		fclose(fp);
	}

	va_end(args);
#endif
}

static void Idle
(
	void
)
{
	msleep(1);
	glutPostRedisplay();
}

static void Timer
(
	int value
)
{
	unsigned long now;

	// Reset the timer.
	///////////////////
	glutTimerFunc(10, Timer, 0);

	// Get the current time.
	////////////////////////
	now = current_time();

	// Let the subsystems think.
	////////////////////////////
	if(host)
		ServerThink(now);
	if(!dedicated)
		ClientThink(now);
	DisplayThink(now);
}

static GHTTPBool NaminatorCallback
(
	GHTTPRequest request,
	GHTTPResult result,
	char * buffer,
	GHTTPByteCount bufferLen,
	void * param
)
{
	if(result == GHTTPSuccess)
	{
		char * str;

		// Set the nick.
		////////////////
		strncpy(localNick, buffer, MAX_NICK);
		localNick[MAX_NICK - 1] = '\0';

		// Cap off the newline.
		///////////////////////
		str = strchr(localNick, '\n');
		if(str)
			*str = '\0';

		// Strip out stuff that'll mess up our
		// key\value message passing.
		//////////////////////////////////////
		while(str = strchr(localNick, '\\'))
			*str = '/';

		printf("%s\n", localNick);
	}
	else
	{
		printf("Error\n");
	}

	return GHTTPTrue;
}

static void ParseArgs
(
	int argc,
	char ** argv
)
{
	GT2Bool gotNick = GT2False;
	int i;

	for(i = 1 ; i < argc ; i++)
	{
		// Connect.
		///////////
		if(strncasecmp(argv[i], "-c", 2) == 0)
		{
			if(++i < argc)
			{
				// Get the address to connect to.
				/////////////////////////////////
				strcpy(serverAddress, argv[i]);
				if(!strchr(serverAddress, ':'))
					strcat(serverAddress, PORT_STRING);
				host = GT2False;
			}
		}
		// Fullscreen.
		//////////////
		else if(strcasecmp(argv[i], "-full") == 0)
		{
			fullScreen = GT2True;
		}
		// Nick.
		////////
		else if((strcasecmp(argv[i], "-nick") == 0) ||
			(strcasecmp(argv[i], "-name") == 0))
		{
			if(++i < argc)
			{
				char * str;

				// Set the nick.
				////////////////
				strncpy(localNick, argv[i], MAX_NICK);
				localNick[MAX_NICK - 1] = '\0';

				// Strip out stuff that'll mess up our
				// key\value message passing.
				//////////////////////////////////////
				while(str = strchr(localNick, '\\'))
					*str = '/';

				// We got a nick.
				/////////////////
				gotNick = GT2True;
			}
		}
		// Dedicated server.
		////////////////////
		else if(strcasecmp(argv[i], "-dedicated") == 0)
		{
			dedicated = GT2True;
		}
		// Width.
		/////////
		else if(strcasecmp(argv[i], "-width") == 0)
		{
			if(++i < argc)
				screenWidth = atoi(argv[i]);
		}
		// Height.
		//////////
		else if(strcasecmp(argv[i], "-height") == 0)
		{
			if(++i < argc)
				screenHeight = atoi(argv[i]);
		}
	}

	// If we didn't get a nick, get a naminator nick.
	/////////////////////////////////////////////////
	if(!gotNick)
	{
		printf("Getting naminator name...");
		ghttpGetFile("http://www.planetquake.com/excessive/name.asp", GHTTPTrue, NaminatorCallback, NULL);
	}
}

static GT2Bool Initialize
(
	void
)
{
	// Init the display.
	////////////////////
	InitializeDisplay();

	// Init input handling.
	///////////////////////
	InitializeInput();

	// Init sound.
	//////////////
	InitializeSound();

	// Start the server if we're hosting.
	/////////////////////////////////////
	if(host)
	{
		if(!InitializeServer())
		{
			printf("Failed to initialize server!\n");
			return GT2False;
		}
	}

	// Start the client if not dedicated.
	/////////////////////////////////////
	if(!dedicated)
	{
		if(!InitializeClient())
		{
			printf("Failed to initialize client!\n");
			return GT2False;
		}
	}

	return GT2True;
}

int main
(
	int argc,
	char ** argv
)
{
	srand(time(NULL));
	glutInit(&argc, argv);
	ParseArgs(argc, argv);
	glutTimerFunc(10, Timer, 0);
	glutIdleFunc(Idle);
	if(!Initialize())
		return 1;
	glutMainLoop();
	return 0;
}
