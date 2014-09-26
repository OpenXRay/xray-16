/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include "gt2aMain.h"
#include "gt2aDisplay.h"
#include "gt2aClient.h"
#include "gt2aMath.h"
#include "gt2aInput.h"
#include "gt2aLogic.h"
#include "TGAFile.h"

#define TITLE                  "GameSpy GT2Action by Dan 'Mr. Pants' Schoenblum"
#define FONT                   GLUT_STROKE_ROMAN
#define TEXT_SCALE             10
#define CHAT_LINES             3
#define CHAT_SCROLL_TIME       (3.5 * 1000)
#define NUM_STARS              1000
#define FRAME_HISTORY_LEN      250
#define NUM_ROCKET_TEXTURES    4
#define NUM_EXPLOSION_TEXTURES 2
#define NUM_SHIP_TEXTURES      2
#define NUM_ASTEROID_TEXTURES  3
#define NUM_MINE_TEXTURES      3
#define NUM_SPINNER_TEXTURES   3
#define WINDOW_MIN             0
#define WINDOW_MAX             10000
#define POSITION_HISTORY_LEN   5
#define MAX_FLICKER            50
#define STARS_SCALE            4
#define RADAR_SIZE             1500
#define FULL_VIEW              (20000.0 * Zoom)
#define HALF_VIEW              (FULL_VIEW / 2.0)
#define ZOOM_IN_SPEED          0.4
#define ZOOM_OUT_SPEED         0.4
#define ZOOM_MIN               1.0
#define ZOOM_MAX               1.5

typedef struct Star
{
	V2f position;
	V3b color;
	byte alpha;
	byte flicker;
} Star;

GT2Bool fullScreen;

GT2Bool DrawGraphsOption;
GT2Bool DrawUpdateTimeOption = GT2True;
GT2Bool DrawUpdateLengthOption = GT2True;
GT2Bool DrawFrameTimeOption = GT2True;
GT2Bool DrawMarksOption = GT2True;
GT2Bool DrawFPSOption;
GT2Bool DrawWithPredictionOption = GT2True;
GT2Bool DrawBackgroundOption = GT2True;
GT2Bool DrawStarsOption = GT2True;
GT2Bool DrawBoundingCirclesOption = GT2False;
GT2Bool DrawRadarOption = GT2True;
float RadarScaleOption = 1;
float RadarPointScaleOption = 2;
GT2Bool ViewClippingOption = GT2True;

int screenWidth = 500;
int screenHeight = 500;

static char chatLines[CHAT_LINES][CHAT_MAX];
static unsigned long lastChatScroll;

static unsigned long Now;
static unsigned long Diff;

static GLuint backgroundTexture;
static GLuint shipTextures[NUM_SHIP_TEXTURES];
static GLuint explosionTextures[NUM_EXPLOSION_TEXTURES];
static GLuint rocketTextures[NUM_ROCKET_TEXTURES];
static GLuint asteroidTextures[NUM_ASTEROID_TEXTURES];
static GLuint mineTextures[NUM_MINE_TEXTURES];
static GLuint spinnerTextures[NUM_SPINNER_TEXTURES];

static Star stars[NUM_STARS];

static int FrameHistory[FRAME_HISTORY_LEN];
static int FrameHistoryStart;

static V2f localPosition;

static float Zoom = 1.0;

static void ScrollChat
(
	void
)
{
	int i;

	// First find where to start the copying.
	/////////////////////////////////////////
	for(i = 0 ; i < (CHAT_LINES - 1) ; i++)
		if(!chatLines[i][0])
			break;

	// Move all lines 1 step towards the end of the array.
	//////////////////////////////////////////////////////
	for( ; i > 0 ; i--)
		strcpy(chatLines[i], chatLines[i - 1]);

	// Clear the first line.
	////////////////////////
	chatLines[0][0] = '\0';
}

static void RemoveOldestChat
(
	void
)
{
	int i;

	// No lines?
	////////////
	if(!chatLines[0][0])
		return;

	// Find the oldest line.
	////////////////////////
	for(i = (CHAT_LINES - 1) ; !chatLines[i][0] && (i > 0) ; i--);

	// Remove it.
	/////////////
	chatLines[i][0] = '\0';
}

static void AddChatLine
(
	const char * message
)
{
	// Do scrolling.
	////////////////
	ScrollChat();

	// Copy in the new line.
	////////////////////////
	strncpy(chatLines[0], message, CHAT_MAX);
	chatLines[0][CHAT_MAX - 1] = '\0';

	// Update the scroll time.
	//////////////////////////
	lastChatScroll = current_time();
}

static int GetStringWidth
(
	const char * string
)
{
	int width = 0;
	
	if(!string)
		return 0;

	while(*string)
		width += glutStrokeWidth(FONT, *string++);

	return (width * TEXT_SCALE);
}

static void DrawString
(
	const char * string,
	int x,
	int y,
	const V3b color,
	float scale
)
{
	scale *= TEXT_SCALE;
	glColor3ubv(color);
	glPushMatrix();
		glTranslatef(x, y, 0);
		glScalef(scale, scale, 1);
		while(*string)
			glutStrokeCharacter(FONT, *string++);
	glPopMatrix();
}

static int GetCharacterWidth
(
	char ch
)
{
	int width;

	width = glutStrokeWidth(FONT, ch);
	width *= TEXT_SCALE;

	return width;
}

static void DrawCharacter
(
	char ch,
	int x,
	int y,
	const V3b color
)
{
	glColor3ubv(color);
	glPushMatrix();
		glTranslatef(x, y, 0);
		glScalef(TEXT_SCALE, TEXT_SCALE, 1);
		glutStrokeCharacter(FONT, ch);
	glPopMatrix();
}

void DisplayChat
(
	const char * message
)
{
	// Add this to the chat buffer.
	///////////////////////////////
	AddChatLine(message);
}

static void DrawChat
(
	void
)
{
	const char * str;
	int x;
	int y;
	int i;
	float scale;

	// Settings.
	////////////
	x = 100;
	y = 200;
	scale = 0.3;

	// Draw all the chat messages.
	//////////////////////////////
	for(i = 0 ; (i < CHAT_LINES) && chatLines[i][0] ; i++)
	{
		DrawString(chatLines[i], x, y, White, scale);
		y += 500;
	}

	// Get the current chat buffer.
	///////////////////////////////
	str = GetChatBuffer();
	if(str)
		DrawString(str, x, WINDOW_MAX - 500, White, scale);
}

static void DrawBoundingCircle
(
	float radius
)
{
	// Check the bounding circles option.
	/////////////////////////////////////
	if(DrawBoundingCirclesOption)
	{
		int i;
		V2f point;

		// Draw in white.
		/////////////////
		glColor3f(1, 1, 1);

		// Draw the circle (circle, sphere, whatever).
		//////////////////////////////////////////////
#if 1
		glBegin(GL_LINE_LOOP);
			for(i = 0 ; i < 360 ; i += 30)
			{
				RotationToVector(point, i);
				ScaleV2f(point, point, radius);
				glVertex2fv(point);
			}
		glEnd();
#else
		glutWireSphere(radius, 16, 16);
#endif

		// Draw the forward line.
		/////////////////////////
		glBegin(GL_LINES);
			glVertex2f(0, 0);
			glVertex2f(0, radius * 2);
		glEnd();
	}
}

static void DrawPlayer
(
	Player * player,
	const V2f position,
	float rotation
)
{
	int width;
	static const float textScale = 0.25;
	V2f textPosition;
	float diff;
	static char text[MAX_NICK + 32];

	// Get the text to show.
	////////////////////////
	sprintf(text, "%s: %d", player->nick, player->score);

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
		glTranslatef(position[0], position[1], 0);

		width = GetStringWidth(text);
		width *= textScale;
		textPosition[0] = -(width / 2);
#if 0
		textPosition[1] = 700;
#else
		if((position[0] + textPosition[0]) < 20)
			textPosition[0] = (20 - position[0]);
		diff = (MAP_MAX - (position[0] + textPosition[0] + width));
		if(diff < 0)
			textPosition[0] += diff;
		if(position[1] < (MAP_MAX - 1000))
			textPosition[1] = 700;
		else
			textPosition[1] = -1000;
#endif
		DrawString(text, textPosition[0], textPosition[1], White, textScale);

		if(!player->dead)
		{
			glRotatef(rotation, 0, 0, -1);

			glPushMatrix();
				if(player->roll)
				{
					glScalef(1, 1, .005);
					glRotatef(35 * player->roll, 0, 1, 0);
				}

				glEnable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);

				if(player->motion)
					glBindTexture(GL_TEXTURE_2D, shipTextures[1]);
				else
					glBindTexture(GL_TEXTURE_2D, shipTextures[0]);

				glColor4f(1, 1, 1, 1);

				glBegin(GL_QUADS);
					glTexCoord2f(0, 0);
					glVertex2f(-500, -300);
					glTexCoord2f(1, 0);
					glVertex2f(500, -300);
					glTexCoord2f(1, 1);
					glVertex2f(500, 700);
					glTexCoord2f(0, 1);
					glVertex2f(-500, 700);
				glEnd();

				glDisable(GL_BLEND);
				glDisable(GL_TEXTURE_2D);
			glPopMatrix();
		}

		DrawBoundingCircle(PLAYER_RADIUS);
	glPopMatrix();
}

static void Predict
(
	V2f newPosition,
	float * rotation,
	Player * player
)
{
	GT2Bool local;

	assert(newPosition);
	assert(rotation);
	assert(player);

	// Is this the local player?
	////////////////////////////
	local = (player == &players[localIndex]);

	// Should we predict?
	/////////////////////
	if(DrawWithPredictionOption && !player->dead && player->motion)
	{
		// Predict new position.
		////////////////////////
		ComputeNewPosition(
			newPosition,
			player->position,
			player->motion,
			player->rotation,
			Diff,
			PLAYER_SPEED,
			GT2True);

		// Predict rotation if not local.
		/////////////////////////////////
		if(!local)
			*rotation = ComputeNewRotation(player->rotation, player->turning, Diff, PLAYER_TURN_SPEED);
	}
	else
	{
		// Just use the actual position.
		////////////////////////////////
		CopyV2f(newPosition, player->position);

		// Use the real rotation if not local.
		//////////////////////////////////////
		if(!local)
			*rotation = player->rotation;
	}

	// If local, use our rotation.
	//////////////////////////////
	if(local)
		*rotation = localRotation;
}

static void DrawPlayers
(
	void
)
{
	int i;
	Player * player;
	V2f position;
	float rotation;
	float distance;

	// First draw the other players.
	////////////////////////////////
	for(i = 0 ; i < MAX_PLAYERS ; i++)
	{
		player = &players[i];

		// Check if we need to draw the player.
		///////////////////////////////////////
		if(player->used && (i != localIndex))
		{
			// Predict the new position.
			////////////////////////////
			Predict(position, &rotation, player);

			// Check if the players are close enough.
			/////////////////////////////////////////
			if(ViewClippingOption)
			{
				distance = DistanceV2f(localPosition, position);
				if(distance >= FULL_VIEW)
					continue;
			}

			// Do the actual drawing.
			/////////////////////////
			DrawPlayer(player, position, rotation);
		}
	}

	// Draw the local player.
	/////////////////////////
	assert((localIndex >= 0) && (localIndex < MAX_PLAYERS));
	player = &players[localIndex];

	// Do the actual drawing.
	// The position was already set in SetMapView.
	//////////////////////////////////////////////
	DrawPlayer(player, localPosition, localRotation);
}

static void DrawRocket
(
	CObject * rocket
)
{
	V2f position;

	// Are we using prediction?
	///////////////////////////
	if(DrawWithPredictionOption)
	{
		// Predict new position.
		////////////////////////
		ComputeNewPosition(
			position,
			rocket->position,
			FORWARD,
			rocket->rotation,
			Diff,
			ROCKET_SPEED,
			GT2False);
	}
	else
	{
		// Just use the actual position.
		////////////////////////////////
		CopyV2f(position, rocket->position);
	}

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
		glTranslatef(position[0], position[1], 0);

		glRotatef(rocket->rotation, 0, 0, -1);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		glBindTexture(GL_TEXTURE_2D, rocketTextures[RandomInt(0, NUM_ROCKET_TEXTURES - 1)]);

		glColor4f(1, 1, 1, 1);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(-350, -350);
			glTexCoord2f(1, 0);
			glVertex2f(350, -350);
			glTexCoord2f(1, 1);
			glVertex2f(350, 350);
			glTexCoord2f(0, 1);
			glVertex2f(-350, 350);
		glEnd();

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);

		DrawBoundingCircle(ROCKET_RADIUS);
	glPopMatrix();
}

static void DrawMine
(
	CObject * mine
)
{
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
		glTranslatef(mine->position[0], mine->position[1], 0);

		glRotatef(mine->rotation, 0, 0, -1);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		glBindTexture(GL_TEXTURE_2D, mineTextures[(int)mine->position[0] % NUM_MINE_TEXTURES]);

		glColor4f(1, 1, 1, 1);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(-800, -800);
			glTexCoord2f(1, 0);
			glVertex2f(800, -800);
			glTexCoord2f(1, 1);
			glVertex2f(800, 800);
			glTexCoord2f(0, 1);
			glVertex2f(-800, 800);
		glEnd();
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);

		DrawBoundingCircle(MINE_RADIUS);
	glPopMatrix();
}

static void DrawAsteroid
(
	CObject * asteroid
)
{
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
		glTranslatef(asteroid->position[0], asteroid->position[1], 0);

		glRotatef(asteroid->rotation, 0, 0, -1);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		//glBindTexture(GL_TEXTURE_2D, asteroidTextures[(int)asteroid->position[0] % NUM_ASTEROID_TEXTURES]);
		glBindTexture(GL_TEXTURE_2D, asteroidTextures[1]);

		glColor4f(1, 1, 1, 1);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(-800, -800);
			glTexCoord2f(1, 0);
			glVertex2f(800, -800);
			glTexCoord2f(1, 1);
			glVertex2f(800, 800);
			glTexCoord2f(0, 1);
			glVertex2f(-800, 800);
		glEnd();
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);

		DrawBoundingCircle(ASTEROID_RADIUS);
	glPopMatrix();
}

static void DrawExplosion
(
	CObject * explosion
)
{
	unsigned long totalTime;

	totalTime = (explosion->totalTime + Diff);

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
		glTranslatef(explosion->position[0], explosion->position[1], 0);

		glPushMatrix();
			glScalef(totalTime / 100.0, totalTime / 100.0, 1);

			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);

			glColor4f(1, 1, 1, 1);

			glBindTexture(GL_TEXTURE_2D, explosionTextures[1]);

			glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-700, -700);
				glTexCoord2f(1, 0);
				glVertex2f(700, -700);
				glTexCoord2f(1, 1);
				glVertex2f(700, 700);
				glTexCoord2f(0, 1);
				glVertex2f(-700, 700);
			glEnd();

			glRotatef(totalTime, 0, 0, -1);

			glBindTexture(GL_TEXTURE_2D, explosionTextures[0]);

			glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-700, -700);
				glTexCoord2f(1, 0);
				glVertex2f(700, -700);
				glTexCoord2f(1, 1);
				glVertex2f(700, 700);
				glTexCoord2f(0, 1);
				glVertex2f(-700, 700);
			glEnd();

			glDisable(GL_BLEND);
			glDisable(GL_TEXTURE_2D);
		glPopMatrix();

		DrawBoundingCircle(EXPLOSION_RADIUS);
	glPopMatrix();
}

static void DrawObjects
(
	void
)
{
	int i;
	CObject * object;
	float distance;

	// Draw all the objects.
	////////////////////////
	for(i = 0 ; i < MAX_OBJECTS ; i++)
	{
		object = &cObjects[i];

		// Is the object in use?
		////////////////////////
		if(object->used)
		{
			if(ViewClippingOption)
			{
				// Is the player close enough to see it?
				////////////////////////////////////////
				distance = DistanceV2f(localPosition, object->position);
				if(distance >= FULL_VIEW)
					continue;
			}

			// Draw it based on type.
			/////////////////////////
			if(object->type == ObjectRocket)
				DrawRocket(object);
			else if(object->type == ObjectMine)
				DrawMine(object);
			else if(object->type == ObjectAsteroid)
				DrawAsteroid(object);
			else if(object->type == ObjectExplosion)
				DrawExplosion(object);
		}
	}
}

static void DrawConnecting
(
	void
)
{
	static const char * text = "Connecting";
	static int width = -1;
	static const V3b textColor = { 0, 204, 0};
	static unsigned long lastSpin;
	static int spinnerIndex = -1;
	static const char * spinnerText = "|/-\\";
	static const V3b spinnerColor = { 0, 204, 0 };
	char spinnerChar;
	int spinnerX;

	// Get the width of the string if we don't already have it.
	///////////////////////////////////////////////////////////
	if(width == -1)
		width = GetStringWidth(text);

	// Draw the string.
	///////////////////
	DrawString(text, (WINDOW_MAX - width) / 2, (WINDOW_MAX / 2), textColor, 1);

	// Init if its the first time.
	//////////////////////////////
	if(spinnerIndex == -1)
	{
		spinnerIndex = 0;
		lastSpin = Now;
	}
	// Check if its time to spin the spinner.
	/////////////////////////////////////////
	else if((Now - lastSpin) >= 250)
	{
		spinnerIndex++;
		spinnerIndex %= 4;
		lastSpin = Now;
	}

	// Get the char to show.
	////////////////////////
	spinnerChar = spinnerText[spinnerIndex];

	// Get the x position of the char.
	//////////////////////////////////
	spinnerX = ((WINDOW_MAX - GetCharacterWidth(spinnerChar)) / 2);

	// Draw the spinner.
	////////////////////
	DrawCharacter(spinnerChar, spinnerX, 3000, spinnerColor);
}

static int ReadLittleInt
(
	FILE * file
)
{
	byte bytes[4];
	int i;

	if(fread(bytes, 1, 4, file) != 4)
		return -1;

	i = bytes[0];
	i |= (bytes[1] << 8);
	i |= (bytes[2] << 16);
	i |= (bytes[3] << 24);

	return i;
}

static GT2Bool LoadTexture
(
	const char * filename,
	GLuint * texture,
	GT2Bool useAlpha
)
{
	int width;
	int height;
	GLubyte * bytes;
	int len;
	int i;
	GLubyte temp;
	int format;

	// Load the file.
	/////////////////
	bytes = LoadTGAFile(filename, &width, &height);
	if(!bytes)
		return GT2False;

	// Convert from BGRA to RGBA.
	/////////////////////////////
	len = (width * height * 4);
	for(i = 0 ; i < len ; i += 4)
	{
		temp = bytes[i];
		bytes[i] = bytes[i + 2];
		bytes[i + 2] = temp;
	}

	// Generate and bind the texture.
	/////////////////////////////////
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);

	// Load it from memory.
	///////////////////////
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if(useAlpha)
		format = 4;
	else
		format = 3;
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	// Set to decal mode.
	/////////////////////
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Setup the min/mag filters.
	/////////////////////////////
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	free(bytes);

	return GT2True;
}

static void DrawBackground
(
	void
)
{
	if(!DrawBackgroundOption)
		return;

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, backgroundTexture);

	glColor4f(1, 1, 1, 1);

	glBegin(GL_QUADS);
#if 0
		glTexCoord2f(0, 0);
		glVertex2f(MAP_MIN, MAP_MIN);
		glTexCoord2f(1, 0);
		glVertex2f(MAP_MAX, MAP_MIN);
		glTexCoord2f(1, 1);
		glVertex2f(MAP_MAX, MAP_MAX);
		glTexCoord2f(0, 1);
		glVertex2f(MAP_MIN, MAP_MAX);
#else
		glTexCoord2f(0.01, 0.01);
		glVertex2f(MAP_MIN, MAP_MIN);
		glTexCoord2f(0.99, 0.01);
		glVertex2f(MAP_MAX, MAP_MIN);
		glTexCoord2f(0.99, 0.99);
		glVertex2f(MAP_MAX, MAP_MAX);
		glTexCoord2f(0.01, 0.99);
		glVertex2f(MAP_MIN, MAP_MAX);
#endif
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

static void InitStar
(
	unsigned long now,
	Star * star
)
{
	int color;

	star->position[0] = RandomFloat(MAP_MIN, MAP_MAX, GT2True);
	star->position[1] = RandomFloat(MAP_MIN, MAP_MAX, GT2True);

	color = RandomInt(0, 19);
	if(color < 15)
		CopyV3b(star->color, White);
	else if(color < 17)
		CopyV3b(star->color, Red);
	else if(color < 19)
		CopyV3b(star->color, Yellow);
	else
		CopyV3b(star->color, Blue);
	star->alpha = RandomInt(0, 255);
	star->flicker = RandomInt(1, MAX_FLICKER);
}

static void DrawStars
(
	void
)
{
	int i;
	int alpha;
	V2f position;

	if(!DrawStarsOption)
		return;

	// Figure out the eye position.
	///////////////////////////////
	CopyV2f(position, localPosition);
	if(position[0] < HALF_VIEW)
		position[0] = HALF_VIEW;
	else if(position[0] > (MAP_MAX - HALF_VIEW))
		position[0] = (MAP_MAX - HALF_VIEW);
	if(position[1] < HALF_VIEW)
		position[1] = HALF_VIEW;
	else if(position[1] > (MAP_MAX - HALF_VIEW))
		position[1] = (MAP_MAX - HALF_VIEW);

	// Draw the stars.
	//////////////////
	glEnable(GL_BLEND);
	glPushMatrix();
		glTranslatef(position[0] / STARS_SCALE, position[1] / STARS_SCALE, 0);

		glBegin(GL_POINTS);
			for(i = 0 ; i < NUM_STARS ; i++)
			{
				alpha = stars[i].alpha;
				alpha += RandomInt(-stars[i].flicker, stars[i].flicker);
				alpha = ClampInt(alpha, 0, 255);
				glColor4ub(stars[i].color[0], stars[i].color[1], stars[i].color[2], (byte)alpha);
				glVertex2fv(stars[i].position);
			}
		glEnd();
	glPopMatrix();
	glDisable(GL_BLEND);
}

static void DrawSpinner
(
	void
)
{
	float rotation;
	float scale;

	scale = (Now % 1001);
	scale -= 50;
	scale /= 50;

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
		glTranslatef(MAP_HALF, MAP_HALF, 0);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

#if 1
		glPushMatrix();
			rotation = (Now % 360);
			glRotatef(rotation, 0, 0, 1);
			glBindTexture(GL_TEXTURE_2D, spinnerTextures[0]);
			glColor4f(1, 1, 1, 1);
			glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-SPINNER_COORD, -SPINNER_COORD);
				glTexCoord2f(1, 0);
				glVertex2f(SPINNER_COORD, -SPINNER_COORD);
				glTexCoord2f(1, 1);
				glVertex2f(SPINNER_COORD, SPINNER_COORD);
				glTexCoord2f(0, 1);
				glVertex2f(-SPINNER_COORD, SPINNER_COORD);
			glEnd();
		glPopMatrix();
#endif

		glPushMatrix();
			rotation = (Now % (360 * 8));
			rotation /= 8;
			glRotatef(rotation, 0, 0, 1);
			glScalef(scale, -scale, 1);
			glBindTexture(GL_TEXTURE_2D, spinnerTextures[2]);
			glColor4f(1, 1, 1, 0.8);
			glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-SPINNER_COORD, -SPINNER_COORD);
				glTexCoord2f(1, 0);
				glVertex2f(SPINNER_COORD, -SPINNER_COORD);
				glTexCoord2f(1, 1);
				glVertex2f(SPINNER_COORD, SPINNER_COORD);
				glTexCoord2f(0, 1);
				glVertex2f(-SPINNER_COORD, SPINNER_COORD);
			glEnd();
		glPopMatrix();

		glPushMatrix();
			rotation = (Now % (360 * 7));
			rotation /= 7;
			glRotatef(rotation, 0, 0, 1);
			glScalef(scale, -scale, 1);
			glBindTexture(GL_TEXTURE_2D, spinnerTextures[1]);
			glColor4f(1, 1, 1, 0.8);
			glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-SPINNER_COORD, -SPINNER_COORD);
				glTexCoord2f(1, 0);
				glVertex2f(SPINNER_COORD, -SPINNER_COORD);
				glTexCoord2f(1, 1);
				glVertex2f(SPINNER_COORD, SPINNER_COORD);
				glTexCoord2f(0, 1);
				glVertex2f(-SPINNER_COORD, SPINNER_COORD);
			glEnd();
		glPopMatrix();

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);

		DrawBoundingCircle(SPINNER_RADIUS);
	glPopMatrix();
}

static void DrawGraphs
(
	void
)
{
	int i;
	int diff;
	int len;
	int maxDiff = 0;
	int maxLen = 0;

	if(updateHistoryStart == -1)
		return;

	if(!DrawGraphsOption)
		return;

	glPushMatrix();
		glScalef(20, 20, 1);
		if(DrawUpdateTimeOption)
		{
			glColor3ubv(Green);
			glBegin(GL_LINE_STRIP);
				for(i = 0 ; i < UPDATE_HISTORY_LEN ; i++)
				{
					diff = updateHistory[(updateHistoryStart + i) % UPDATE_HISTORY_LEN].diff;

					if(diff > maxDiff)
						maxDiff = diff;

					if(diff == -1)
					{
						glColor3ubv(Red);
						glVertex2f(i * 2, 50);
						glColor3ubv(Green);
					}
					else
					{
						if(diff == -1)
							diff = 0;
						glVertex2f(i * 2, diff / 2);
					}
				}
			glEnd();
		}

		if(DrawUpdateLengthOption)
		{
			glColor3ubv(Purple);
			glBegin(GL_LINE_STRIP);
				for(i = 0 ; i < UPDATE_HISTORY_LEN ; i++)
				{
					len = updateHistory[(updateHistoryStart + i) % UPDATE_HISTORY_LEN].len;

					if(len > maxLen)
						maxLen = len;

					glVertex2f(i * 2, len / 8);
				}
			glEnd();
		}

		if(DrawFrameTimeOption)
		{
			glColor3ubv(Yellow);
			glBegin(GL_LINE_STRIP);
				for(i = 0 ; i < FRAME_HISTORY_LEN ; i++)
				{
					diff = FrameHistory[(FrameHistoryStart + i) % FRAME_HISTORY_LEN];
					glVertex2f(i * 2, diff / 2);
				}
			glEnd();
		}

		if(DrawMarksOption)
		{
			glColor3ubv(White);
			glBegin(GL_LINES);
				for(i = 0 ; i < screenHeight; i += 25)
				{
					glVertex2f(screenWidth - 10, i);
					glVertex2f(screenWidth, i);
				}
			glEnd();
		}

	glPopMatrix();
}

static void DrawFPS
(
	unsigned long diff
)
{
	if(DrawFPSOption && diff)
	{
		static int counter;
		static char buf[64];
		if(++counter == 10)
		{
			counter = 0;
			sprintf(buf, "FPS: %d", 1000 / diff);
		}
		DrawString(buf, WINDOW_MAX - 1500, 600, Grey, 0.2);
	}

	{
	char buf[64];
	sprintf(buf, "%d asteroid%s", ClientNumAsteroids, 
		(ClientNumAsteroids==1) ? "" : "s");
	DrawString(buf, WINDOW_MAX - 3500, 600, Grey, 0.2);
	}
}

static void DrawRadar
(
	void
)
{
	int i;
	Player * player;
	CObject * object;
	GLdouble plane0[4] = { 0, 1, 1, 0 };
	GLdouble plane1[4] = { 1, 0, 1, 0 };

	if(!DrawRadarOption)
		return;

	glPushMatrix();
		// Move to the upper-right corner.
		//////////////////////////////////
		glTranslatef(
			WINDOW_MAX - (RADAR_SIZE * RadarScaleOption),
			WINDOW_MAX - (RADAR_SIZE * RadarScaleOption),
			0);

		// Scale so we can draw in map space.
		/////////////////////////////////////
		glScalef(
			RADAR_SIZE / MAP_MAX * RadarScaleOption,
			RADAR_SIZE / MAP_MAX * RadarScaleOption,
			1);

#if 1
		// Enable textures.
		///////////////////
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glColor4f(1, 1, 1, 0.5);

		// Draw a background.
		/////////////////////
		glBindTexture(GL_TEXTURE_2D, backgroundTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(0, 0);
			glTexCoord2f(1, 0);
			glVertex2f(MAP_MAX, 0);
			glTexCoord2f(1, 1);
			glVertex2f(MAP_MAX, MAP_MAX);
			glTexCoord2f(0, 1);
			glVertex2f(0, MAP_MAX);
		glEnd();

		// Draw the spinner.
		////////////////////
		glPushMatrix();
			glTranslatef(MAP_HALF, MAP_HALF, 0);
			glRotatef(Now % 3600, 0, 0, 1);

			glBindTexture(GL_TEXTURE_2D, spinnerTextures[0]);
			glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-SPINNER_COORD, -SPINNER_COORD);
				glTexCoord2f(1, 0);
				glVertex2f(SPINNER_COORD, -SPINNER_COORD);
				glTexCoord2f(1, 1);
				glVertex2f(SPINNER_COORD, SPINNER_COORD);
				glTexCoord2f(0, 1);
				glVertex2f(-SPINNER_COORD, SPINNER_COORD);
			glEnd();
		glPopMatrix();

		// Disable textures.
		////////////////////
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
#endif

		// Draw a box around the radar.
		///////////////////////////////
		glColor3ubv(Yellow);
		glBegin(GL_LINE_STRIP);
			glVertex2f(0, MAP_MAX);
			glVertex2f(0, 0);
			glVertex2f(MAP_MAX, 0);
		glEnd();

		// Setup the clipping planes.
		/////////////////////////////
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
		glClipPlane(GL_CLIP_PLANE0, plane0);
		glClipPlane(GL_CLIP_PLANE1, plane1);

		// Scale points.
		////////////////
		if(RadarPointScaleOption != 1)
			glPointSize(RadarPointScaleOption);

		glBegin(GL_POINTS);
			for(i = 0 ; i < MAX_OBJECTS ; i++)
			{
				object = &cObjects[i];

				if(!object->used)
					continue;

				if(object->type == ObjectRocket)
					glColor3ubv(Red);
				else if(object->type == ObjectMine)
					glColor3ubv(Yellow);
				else if(object->type == ObjectAsteroid)
					glColor3ubv(White);
				else
					continue;

				glVertex2fv(object->position);
			}

			glColor3ubv(Orange);
			for(i = 0 ; i < MAX_PLAYERS ; i++)
			{
				if(i == localIndex)
					continue;

				player = &players[i];

				if(!player->used)
					continue;

				glVertex2fv(player->position);
			}

			glColor3ubv(Green);
			glVertex2fv(players[localIndex].position);
		glEnd();

		// Unscale points.
		//////////////////
		if(RadarPointScaleOption != 1)
			glPointSize(1);
	glPopMatrix();

	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
}

static void SetMapView
(
	void
)
{
	V2f corner;
	Player * player;
	float rotation;
	static V2f positionHistory[POSITION_HISTORY_LEN];
	static int positionHistoryStart = -1;
	int i;

	// Get the local player.
	////////////////////////
	player = &players[localIndex];

	// Predict the local player's position.
	///////////////////////////////////////
	Predict(localPosition, &rotation, player);

	// Put this position in the history.
	////////////////////////////////////
	if(positionHistoryStart == -1)
	{
		positionHistoryStart = 0;
		for(i = 0 ; i < POSITION_HISTORY_LEN ; i++)
			CopyV2f(positionHistory[i], localPosition);
	}
	else
	{
		CopyV2f(positionHistory[positionHistoryStart++], localPosition);
		positionHistoryStart %= POSITION_HISTORY_LEN;
	}

	// Average out the position history to get the display position.
	////////////////////////////////////////////////////////////////
	SetV2f(localPosition, 0, 0);
	for(i = 0 ; i < POSITION_HISTORY_LEN ; i++)
		AddV2f(localPosition, localPosition, positionHistory[i]);
	ScaleV2f(localPosition, localPosition, (1.0 / POSITION_HISTORY_LEN));

	// Set the left and bottom of the view.
	///////////////////////////////////////
	SubScalarV2f(corner, localPosition, HALF_VIEW);

	// Don't show anything outside the map.
	///////////////////////////////////////
	ClampV2f(corner, corner, 0, MAP_MAX - FULL_VIEW);

	glLoadIdentity();
	glViewport(0, 0, screenWidth, screenHeight);
	gluOrtho2D(corner[0], corner[0] + FULL_VIEW, corner[1], corner[1] + FULL_VIEW);
}

static void SetWindowView
(
	void
)
{
	glLoadIdentity();
	glViewport(0, 0, screenWidth, screenHeight);
	gluOrtho2D(0, WINDOW_MAX, 0, WINDOW_MAX);
}

static void Display
(
	void
)
{
	static unsigned long lastDisplay;
	unsigned long now;
	unsigned long diff;

	// Get the current time.
	////////////////////////
	now = current_time();

	// Get the time difference.
	///////////////////////////
	diff = (now - lastDisplay);

	// Update the history.
	//////////////////////
	if(FrameHistoryStart == -1)
		FrameHistoryStart = 0;
	else
	{
		FrameHistory[FrameHistoryStart++] = diff;
		FrameHistoryStart %= FRAME_HISTORY_LEN;
	}

	// New last display.
	////////////////////
	lastDisplay = now;

	// Set the zoom.
	////////////////
	if(localMotion == STILL)
	{
		if(Zoom > ZOOM_MIN)
		{
			Zoom -= (ZOOM_IN_SPEED * diff / 1000);
			if(Zoom < ZOOM_MIN)
				Zoom = ZOOM_MIN;
		}
	}
	else
	{
		if(Zoom < ZOOM_MAX)
		{
			Zoom += (ZOOM_OUT_SPEED * diff / 1000);
			if(Zoom > ZOOM_MAX)
				Zoom = ZOOM_MAX;
		}
	}

	// Clear the color buffer.
	//////////////////////////
	glClear(GL_COLOR_BUFFER_BIT);
	
	// We want to know how long since the last server update.
	/////////////////////////////////////////////////////////
	Now = now;
	Diff = (now - lastServerUpdate);

	// Are we connected to the server?
	//////////////////////////////////
	if(connected)
	{
		// Set the view for drawing in map space.
		/////////////////////////////////////////
		SetMapView();

		// Draw the ground.
		///////////////////
		DrawBackground();

		// Draw some stars.
		///////////////////
		DrawStars();

		// Draw the spinner.
		////////////////////
		DrawSpinner();

		// Draw all the objects.
		////////////////////////
		DrawObjects();

		// Draw all the players.
		////////////////////////
		DrawPlayers();

		// Set the view for drawing in window space.
		////////////////////////////////////////////
		SetWindowView();

		// Draw the radar.
		//////////////////
		DrawRadar();

		// Draw the chat.
		/////////////////
		DrawChat();

		// Draw graphs.
		///////////////
		DrawGraphs();

		// Show the framerate.
		//////////////////////
		DrawFPS(diff);
	}
	else
	{
		// Set the view for drawing in window space.
		////////////////////////////////////////////
		SetWindowView();

		// Draw the connecting screen.
		//////////////////////////////
		DrawConnecting();
	}

	// Saw the front and back buffers.
	//////////////////////////////////
	glutSwapBuffers();
}

static void Reshape
(
	int width,
	int height
)
{
	screenWidth = width;
	screenHeight = height;

	// Setup a simple 2D orthographic view.
	///////////////////////////////////////
	glLoadIdentity();
	glViewport(0, 0, width, height);
	gluOrtho2D(0, MAP_MAX, 0, MAP_MAX);
}

void InitializeDisplay
(
	void
)
{
	int i;
	unsigned long now;
	char buffer[32];

	// Init glut window.
	////////////////////

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);

	if(fullScreen)
	{
		glutGameModeString("640x480:32");
		glutEnterGameMode();
		//glutFullScreen();
		glutSetCursor(GLUT_CURSOR_NONE);
	}
	else
	{
		glutInitWindowSize(screenWidth, screenHeight);
		glutInitWindowPosition(200, 200);
		glutCreateWindow(TITLE);
	}
	// Set display callbacks.
	/////////////////////////
	glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);

	// Set the background color.
	////////////////////////////
	glClearColor(0, 0, 0, 0);

	// Smooth points.
	/////////////////
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	// Don't show the backs of polygons.
	////////////////////////////////////
	glEnable(GL_CULL_FACE);

	// We always use the same blend.
	////////////////////////////////
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Load textures.
	/////////////////
	printf("Loading textures...");
	LoadTexture("images/space.tga", &backgroundTexture, GT2False);
	printf(".");
	for(i = 0 ; i < NUM_SHIP_TEXTURES ; i++)
	{
		sprintf(buffer, "images/ship%d.tga", i);
		LoadTexture(buffer, &shipTextures[i], GT2True);
		printf(".");
	}
	for(i = 0 ; i < NUM_EXPLOSION_TEXTURES ; i++)
	{
		sprintf(buffer, "images/explosion%d.tga", i);
		LoadTexture(buffer, &explosionTextures[i], GT2True);
		printf(".");
	}
	for(i = 0 ; i < NUM_ROCKET_TEXTURES ; i++)
	{
		sprintf(buffer, "images/rocket%d.tga", i);
		LoadTexture(buffer, &rocketTextures[i], GT2True);
		printf(".");
	}
	for(i = 0 ; i < NUM_ASTEROID_TEXTURES ; i++)
	{
		sprintf(buffer, "images/asteroid%d.tga", i);
		LoadTexture(buffer, &asteroidTextures[i], GT2True);
		printf(".");
	}
	for(i = 0 ; i < NUM_MINE_TEXTURES ; i++)
	{
		sprintf(buffer, "images/mine%d.tga", i);
		LoadTexture(buffer, &mineTextures[i], GT2True);
		printf(".");
	}
	for(i = 0 ; i < NUM_MINE_TEXTURES ; i++)
	{
		sprintf(buffer, "images/spinner%d.tga", i);
		LoadTexture(buffer, &spinnerTextures[i], GT2True);
		printf(".");
	}
	printf("\n");

	// Get the time.
	////////////////
	now = current_time();

	// Setup stars.
	///////////////
	for(i = 0 ; i < NUM_STARS ; i++)
		InitStar(now, &stars[i]);

	// Setup frame history.
	///////////////////////
	FrameHistoryStart = -1;
}

void ShutdownDisplay
(
	void
)
{
	if(fullScreen)
	{
		glutLeaveGameMode();
	}
}

void DisplayThink
(
	unsigned long now
)
{
	static unsigned long last;
	unsigned long diff;

	// Get the frame time.
	//////////////////////
	now = current_time();
	diff = (now - last);
	if(diff < 5)
		return;
	last = now;

	// Is it time to scroll the chat?
	/////////////////////////////////
	if((now - lastChatScroll) > CHAT_SCROLL_TIME)
	{
		RemoveOldestChat();
		lastChatScroll = now;
	}

	// Update the frame.
	////////////////////
	glutPostRedisplay();
}