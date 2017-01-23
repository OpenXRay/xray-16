/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#ifndef _GT2ALOGIC_H_
#define _GT2ALOGIC_H_

#include "gt2aServer.h"

#define PLAYER_SPEED            7000  // units/second
#define PLAYER_TURN_SPEED       0.14  // degrees/millisecond
#define PLAYER_RADIUS            200
#define PLAYER_MAX_ASTEROIDS      50
#define DEATH_TIME              1000
#define PRESS_TIME               100  // time between presses

#define MAX_OBJECTS              256
#define ROCKET_RADIUS            250
#define ROCKET_SPEED           10500
#define MINE_TIME              30000
#define MINE_ARM_TIME           1000
#define MINE_TURN_SPEED         0.36  // degrees/millisecond
#define MINE_RADIUS              200//141
#define NUM_ASTEROIDS             50
#define ASTEROID_TURN_SPEED     0.18
#define ASTEROID_TURN_RANGE     0.24
#define ASTEROID_RADIUS          350
#define ASTEROID_SPEED_MIN         0
#define ASTEROID_SPEED_MAX      5000
#define EXPLOSION_RADIUS         900
#define EXPLOSION_TIME           350
#define EXPLOSION_DANGER_TIME    250
#define SPINNER_RADIUS          5000
#define SPINNER_COORD           (SPINNER_RADIUS * 1.414)

typedef enum
{
	ObjectRocket,
	ObjectMine,
	ObjectAsteroid,
	ObjectExplosion,
	NumObjects
} ObjectType;

typedef void (* Think)
(
	struct SObject * self
);

typedef void (* TouchObject)
(
	struct SObject * self,
	struct SObject * object
);

typedef void (* Die)
(
	struct SObject * self
);

typedef void (* TouchClient)
(
	struct SObject * self,
	struct Client * client
);

typedef struct SObject
{
	GT2Bool used;              // If this slot is in use or not.
	ObjectType type;          // The type of object.
	int owner;                // The client that owns this object, or -1.
	V2f position;             // The object's position.
	float rotation;           // The object's rotation.
	unsigned long startTime;  // The time this object was created.
	Think think;              // This object's think function.
	TouchObject touchObject;  // Called when touching another object.
	TouchClient touchClient;  // Called when touching a client.
	Die die;                  // Called when the object dies.
	int count;                // Generic counter.
	float radius;             // This object's radius (for collision).
	GT2Bool explode;           // Explode this object when done thinking.
	GT2Bool remove;            // Remove this object when done thinking.
	
	float heading;			  // The direction the object is moving
	float speed;			  // The speed of the object
} SObject;

extern SObject sObjects[MAX_OBJECTS];
extern int numObjects;

void ObjectsThink
(
	unsigned long now,
	unsigned long diff
);

void ClientPress
(
	int clientIndex,
	const char * button
);

void ClientSpawn
(
	Client * client
);

void InitializeLogic
(
	void
);

#endif