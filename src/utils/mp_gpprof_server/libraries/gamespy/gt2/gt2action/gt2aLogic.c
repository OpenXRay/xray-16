/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include "gt2aMain.h"
#include "gt2aLogic.h"
#include "gt2aMath.h"
#include "gt2aSound.h"
#include "gt2aServer.h"

#define DEATH_MESSAGES 0

SObject sObjects[MAX_OBJECTS];
int numObjects;
static unsigned long Now;
static unsigned long Diff;
static int NumAsteroids;

static void ExplodeObject(SObject * object);
static void InitAsteroid(SObject * asteroid);

static SObject * AddObject
(
	ObjectType type
)
{
	int i;
	SObject * object;

	// Find an open slot.
	/////////////////////
	for(i = 0 ; i < MAX_OBJECTS ; i++)
		if(!sObjects[i].used)
			break;

	// Nothing open?
	////////////////
	if(i == MAX_OBJECTS)
		return NULL;

	// Clear it.
	////////////
	object = &sObjects[i];
	memset(object, 0, sizeof(SObject));
	object->used = GT2True;
	object->type = type;
	object->owner = -1;

	// Set the start time.
	//////////////////////
	object->startTime = Now;

	// One more object.
	///////////////////
	numObjects++;
	assert(numObjects <= MAX_OBJECTS);

	return &sObjects[i];
}

static void RemoveObject
(
	SObject * object
)
{
	assert(object);
	if(!object)
		return;

	// Mark this object for removal.
	////////////////////////////////
	object->remove = GT2True;
}

static void ClientKilled
(
	Client * killed,
	Client * killer
)
{
#if DEATH_MESSAGES
	char buffer[MAX_NICK + MAX_NICK + 32];
#endif

	assert(killed);

	// Is there a killer?
	/////////////////////
	if(killer)
	{
		// Self kill?
		/////////////
		if(killer == killed)
		{
			// Subtract a point.
			////////////////////
			killer->score--;
		}
		else
		{
			// Give the killer a point.
			///////////////////////////
			killer->score++;
		}
	}

	// Kill the killed.
	///////////////////
	killed->dead = GT2True;
	killed->spawnTime = (Now + DEATH_TIME);

	// Play the death sound.
	////////////////////////
	SendSound(SOUND_DIE, -1);

#if DEATH_MESSAGES
	// Let everyone know.
	/////////////////////
	if(killer == killed)
		sprintf(buffer, "%s killed himself", killed->nick);
	else if(killer)
		sprintf(buffer, "%s killed %s", killer->nick, killed->nick);
	else
		sprintf(buffer, "%s was killed", killed->nick);
	BroadcastText(buffer, -1, GT2True);
#endif
}

///////////
//ROCKET //
///////////

static void RocketThink
(
	SObject * rocket
)
{
	assert(rocket);

	// Move the rocket.
	///////////////////
	ComputeNewPosition(
		rocket->position,
		rocket->position,
		FORWARD,
		rocket->rotation,
		Diff,
		ROCKET_SPEED,
		GT2False);

	// Check if we should disappear.
	////////////////////////////////
	if((rocket->position[0] < (MAP_MIN - MAP_EXTRA)) ||
		(rocket->position[0] > (MAP_MAX + MAP_EXTRA)) ||
		(rocket->position[1] < (MAP_MIN - MAP_EXTRA)) ||
		(rocket->position[1] > (MAP_MAX + MAP_EXTRA)))
	{
		RemoveObject(rocket);
	}
}

static void RocketTouchObject
(
	SObject * rocket,
	SObject * object
)
{
	// Don't explode if its another rocket with the same owner.
	///////////////////////////////////////////////////////////
	if((object->type == ObjectRocket) && (object->owner == rocket->owner))
		return;

	// We pass through explosions.
	//////////////////////////////
	if(object->type == ObjectExplosion)
		return;

	// Explode the object.
	//////////////////////
	ExplodeObject(object);

	// Remove the rocket.
	/////////////////////
	RemoveObject(rocket);
}

static void RocketTouchClient
(
	SObject * rocket,
	Client * client
)
{
	// Check for death.
	///////////////////
	if(client->dead)
		return;
	
	// Check for self.
	//////////////////
	if(rocket->owner == client->index)
		return;

	// Kill the client.
	///////////////////
	if(rocket->owner == -1)
		ClientKilled(client, NULL);
	else
		ClientKilled(client, &clients[rocket->owner]);
	
	// Explode the rocket.
	//////////////////////
	ExplodeObject(rocket);
}

//////////
// MINE //
//////////

static void MineThink
(
	SObject * mine
)
{
	// Check for detonate.
	//////////////////////
	if((Now - mine->startTime) > MINE_TIME)
	{
		// Explode the mine.
		////////////////////
		ExplodeObject(mine);
	}
	else
	{
		// Rotate the mine.
		///////////////////
		mine->rotation = ComputeNewRotation(mine->rotation, RIGHT, Diff, MINE_TURN_SPEED);

		// Check if a second has passed.
		////////////////////////////////
		if((int)((Now - mine->startTime) / 1000) > mine->count)
		{
			// One more full second.
			////////////////////////
			mine->count++;

			// Make a sound during the last 3 seconds.
			//////////////////////////////////////////
			if(mine->count >= ((MINE_TIME / 1000) - 3))
				SendSound(SOUND_MINE, -1);
		}
	}
}

static void MineTouchObject
(
	SObject * mine,
	SObject * object
)
{
	// Ignore explosions.
	/////////////////////
	if(object->type == ObjectExplosion)
		return;

	// Blow up the object.
	//////////////////////
	ExplodeObject(object);

	// Remove the mine.
	///////////////////
	RemoveObject(mine);
}

static void MineTouchClient
(
	SObject * mine,
	Client * client
)
{
	// Check if its armed itself.
	/////////////////////////////
	if((Now - mine->startTime) < MINE_ARM_TIME)
		return;

	// Check for death.
	///////////////////
	if(client->dead)
		return;

	// Kill the client.
	///////////////////
	if(mine->owner == -1)
		ClientKilled(client, NULL);
	else
		ClientKilled(client, &clients[mine->owner]);

	// Explode the mine.
	////////////////////
	ExplodeObject(mine);
}

//////////////
// ASTEROID //
//////////////

static void AsteroidThink
(
	SObject * asteroid
)
{
	// Rotate the asteroid.
	///////////////////////
	asteroid->rotation = ComputeNewRotation(asteroid->rotation, LEFT, Diff, ASTEROID_TURN_SPEED);

	// Move the asteroid.
	/////////////////////
	ComputeNewPosition(asteroid->position, asteroid->position, FORWARD, asteroid->heading, Diff, asteroid->speed, GT2False);	

	// If it goes past the edge, loop it around.
	////////////////////////////////////////////
	if(asteroid->position[0] > MAP_MAX + (2*asteroid->radius))
		asteroid->position[0] = MAP_MIN - (2*asteroid->radius);
	if(asteroid->position[0] < MAP_MIN - (2*asteroid->radius))
		asteroid->position[0] = MAP_MAX + (2*asteroid->radius);

	if(asteroid->position[1] > MAP_MAX + (2*asteroid->radius))
		asteroid->position[1] = MAP_MIN - (2*asteroid->radius);
	if(asteroid->position[1] < MAP_MIN - (2*asteroid->radius))
		asteroid->position[1] = MAP_MAX + (2*asteroid->radius);

}

static void AsteroidTouchObject
(
	SObject * asteroid,
	SObject * object
)
{
	// Explode on asteroid impact.
	//////////////////////////////
	if(object->type == ObjectAsteroid)
	{
		ExplodeObject(asteroid);
		ExplodeObject(object);
	}
}

static void AsteroidTouchClient
(
	SObject * asteroid,
	Client * client
)
{
	// Check for death.
	///////////////////
	if(client->dead)
		return;

	// Kill the client.
	///////////////////
	ClientKilled(client, NULL);

	// Explode the asteroid.
	////////////////////////
	ExplodeObject(asteroid);
}

static void SpawnAsteroids
(
	void
)
{
	SObject * asteroid;
	int i;
	
	NumAsteroids = 0;
	for(i = 0 ; i < NUM_ASTEROIDS ; i++)
	{
		asteroid = AddObject(ObjectAsteroid);
		if(asteroid)
			InitAsteroid(asteroid);
	}
}

static void AsteroidDie
(
	SObject * asteroid
)
{
	// One less asteroid.
	/////////////////////
	NumAsteroids--;

	// If they're all gone, make new ones.
	//////////////////////////////////////
	if(!NumAsteroids)
		SpawnAsteroids();
}

static void InitAsteroid
(
	SObject * asteroid
)
{
	asteroid->position[0] = RandomFloat(MAP_MIN, MAP_MAX, GT2True);
	asteroid->position[1] = RandomFloat(MAP_MIN, MAP_MAX, GT2True);
	asteroid->rotation = RandomFloat(0, 360, GT2False);
	asteroid->think = AsteroidThink;
	asteroid->touchObject = AsteroidTouchObject;
	asteroid->touchClient = AsteroidTouchClient;
	asteroid->die = AsteroidDie;
	asteroid->radius = ASTEROID_RADIUS;
	asteroid->speed = RandomFloat(ASTEROID_SPEED_MIN, ASTEROID_SPEED_MAX, GT2False);
	asteroid->heading = asteroid->rotation;

	NumAsteroids++;
}

///////////////
// EXPLOSION //
///////////////

static void ExplosionThink
(
	SObject * explosion
)
{
	// Check for done.
	//////////////////
	if((Now - explosion->startTime) > EXPLOSION_TIME)
	{
		// Delete the explosion.
		////////////////////////
		RemoveObject(explosion);
	}
}

static void ExplosionTouchObject
(
	SObject * explosion,
	SObject * object
)
{
}

static void ExplosionTouchClient
(
	SObject * explosion,
	Client * client
)
{
	// Check if we're still dangerous.
	//////////////////////////////////
	if((Now - explosion->startTime) > EXPLOSION_DANGER_TIME)
		return;

	// Check for death.
	///////////////////
	if(client->dead)
		return;

	// Kill the client.
	///////////////////
	if(explosion->owner == -1)
		ClientKilled(client, NULL);
	else
		ClientKilled(client, &clients[explosion->owner]);
}

static SObject * AddExplosion
(
	V2f position,
	int owner
)
{
	SObject * explosion;

	// Add an explosion.
	////////////////////
	explosion = AddObject(ObjectExplosion);
	if(!explosion)
		return NULL;

	// Setup the explosion.
	///////////////////////
	explosion->owner = owner;
	CopyV2f(explosion->position, position);
	explosion->rotation = RandomFloat(0, 360, GT2False);
	explosion->think = ExplosionThink;
	explosion->touchObject = ExplosionTouchObject;
	explosion->touchClient = ExplosionTouchClient;
	explosion->radius = EXPLOSION_RADIUS;

	// Send a sound effect.
	///////////////////////
	SendSound(SOUND_EXPLOSION, -1);

	return explosion;
}

static void ExplodeObject
(
	SObject * object
)
{
	// Don't explode explosions.
	////////////////////////////
	if(object->type == ObjectExplosion)
		return;

	// Mark this object for explosion.
	//////////////////////////////////
	object->explode = GT2True;

	if(object->type != ObjectAsteroid)
	{
		// Mark for removal.
		////////////////////
		RemoveObject(object);
	}
}

static void ClientThink
(
	Client * client
)
{	
	// Dead?
	////////
	if(client->dead)
	{
		// Time to spawn?
		/////////////////
		if(Now > client->spawnTime)
			ClientSpawn(client);
	}
	else if(client->numAsteroids)
	{
		static V2f spinnerPosition = { MAP_HALF, MAP_HALF };
		float distance;

		// Check for the spinner.
		/////////////////////////
		distance = DistanceV2f(spinnerPosition, client->position);
		if(distance < (PLAYER_RADIUS + SPINNER_RADIUS))
		{
			// Drop off our asteroids.
			//////////////////////////
			client->score += client->numAsteroids;
			client->numAsteroids = 0;
			SendNumAsteroids(client->numAsteroids, client->index);
			SendSound(SOUND_PICKUP, client->index);
		}
	}
}

void ClientPress
(
	int clientIndex,
	const char * button
)
{
	SObject * object;
	Client * client;
	V2f forward;

	assert(clientIndex >= 0);
	assert(clientIndex < MAX_CLIENTS);
	assert(clients[clientIndex].used);

	// Set the Now time.
	////////////////////
	Now = current_time();

	client = &clients[clientIndex];

	// Check the button.
	////////////////////
	if(strcasecmp(button, "mine") == 0)
	{
		// Add a mine object.
		/////////////////////
		object = AddObject(ObjectMine);
		if(object)
		{
			// Set its properties.
			//////////////////////
			object->owner = clientIndex;
			CopyV2f(object->position, client->position);
			object->rotation = client->rotation;
			object->think = MineThink;
			object->touchObject = MineTouchObject;
			object->touchClient = MineTouchClient;
			object->radius = MINE_RADIUS;

			// Move it so its behind the client.
			//////////////////////////////////////
			RotationToVector(forward, object->rotation);
			ScaleV2f(forward, forward, -500);
			AddV2f(object->position, object->position, forward);

			// Play the mine sound.
			///////////////////////
			SendSound(SOUND_MINE, -1);
		}
	}
	else if(strcasecmp(button, "rocket") == 0)
	{
		// Add a rocket object.
		///////////////////////
		object = AddObject(ObjectRocket);
		if(object)
		{
			// Set its properties.
			//////////////////////
			object->owner = clientIndex;
			CopyV2f(object->position, client->position);
			object->rotation = client->rotation;
			object->think = RocketThink;
			object->touchObject = RocketTouchObject;
			object->touchClient = RocketTouchClient;
			object->radius = ROCKET_RADIUS;

			// Move it so its ahead of the client.
			//////////////////////////////////////
			RotationToVector(forward, object->rotation);
			ScaleV2f(forward, forward, 300);
			AddV2f(object->position, object->position, forward);

			// Play the rocket sound.
			/////////////////////////
			SendSound(SOUND_ROCKET, -1);
		}
	}
}

void ObjectThink
(
	SObject * object
)
{
	int i;
	SObject * other;
	Client * client;

	// Think.
	/////////
	object->think(object);

	// Check for client touches.
	////////////////////////////
	if(object->touchClient)
	{
		float range;

		range = (object->radius + PLAYER_RADIUS);

		for(i = 0 ; i < MAX_CLIENTS ; i++)
		{
			client = &clients[i];

			// Check for in use.
			////////////////////
			if(!client->used)
				continue;

			// Check for a touch.
			/////////////////////
			if(DistanceV2f(object->position, client->position) <= range)
			{
				// Touched.
				///////////
				object->touchClient(object, client);
			}
		}
	}

	// Check for object touches.
	////////////////////////////
	if(object->touchObject)
	{
		for(i = 0 ; i < MAX_OBJECTS ; i++)
		{
			other = &sObjects[i];

			// Check for in use.
			////////////////////
			if(!other->used)
				continue;

			// Check for self.
			//////////////////
			if(other == object)
				continue;

			// Check for a touch.
			/////////////////////
			if(DistanceV2f(object->position, other->position) <= (object->radius + other->radius))
			{
				// Touched.
				///////////
				object->touchObject(object, other);
			}
		}
	}
}

void ObjectsThink
(
	unsigned long now,
	unsigned long diff
)
{
	Client * client;
	SObject * object;
	int i;

	Now = now;
	Diff = diff;

	// Go through the list of clients.
	//////////////////////////////////
	for(i = 0 ; i < MAX_CLIENTS ; i++)
	{
		client = &clients[i];

		// Is it a real client?
		///////////////////////
		if(client->used)
		{
			// Think.
			/////////
			ClientThink(client);
		}
	}

	// Think for all the objects.
	/////////////////////////////
	for(i = 0 ; i <  MAX_OBJECTS ; i++)
	{
		object = &sObjects[i];

		// Is the object in use?
		////////////////////////
		if(object->used)
		{
			// Think.
			/////////
			ObjectThink(object);
		}
	}

	// Check for exploding or removed objects.
	//////////////////////////////////////////
	for(i = 0 ; i <  MAX_OBJECTS ; i++)
	{
		object = &sObjects[i];

		// Is the object in use?
		////////////////////////
		if(object->used)
		{
			// Explode the object?
			//////////////////////
			if(object->explode)
			{
				// Create an explosion.
				///////////////////////
				AddExplosion(object->position, object->owner);

				// It exploded.
				///////////////
				object->explode = GT2False;


				// If it is an asteroid, then move it to the edge
				/////////////////////////////////////////////////
				if(object->type == ObjectAsteroid)
				{
					object->position[0] = RandomFloat(MAP_MIN, MAP_MAX, GT2True);
					object->position[1] = MAP_MIN - object->radius;
				}


			}

			// Remove the object?
			/////////////////////
			if(object->remove)
			{
				// Call its die function.
				/////////////////////////
				if(object->die)
					object->die(object);

				if(object->used)
				{
					// We're not using this object anymore.
					///////////////////////////////////////
					object->used = GT2False;

					// One less object.
					///////////////////
					numObjects--;
					assert(numObjects >= 0);
				}
			}
		}
	}
}

void ClientSpawn
(
	Client * client
)
{
	SObject * asteroid;

	// Throw away his asteroids.
	////////////////////////////
	for( ; client->numAsteroids ; client->numAsteroids--)
	{
		asteroid = AddObject(ObjectAsteroid);
		if(asteroid)
		{
			InitAsteroid(asteroid);
			asteroid->position[0] = client->position[0];
			asteroid->position[0] += RandomFloat(-5000, 5000, GT2True);
			asteroid->position[1] = client->position[1];
			asteroid->position[1] += RandomFloat(-5000, 5000, GT2True);
			ClampV2f(asteroid->position, asteroid->position, MAP_MIN, MAP_MAX);
		}
	}
	SendNumAsteroids(client->numAsteroids, client->index);

	// Pick a random starting point.
	////////////////////////////////
	client->position[0] = RandomFloat(MAP_MIN, MAP_MAX, GT2True);
	client->position[1] = RandomFloat(MAP_MIN, MAP_MAX, GT2True);

	// Not dead.
	////////////
	client->dead = GT2False;
}

void InitializeLogic
(
	void
)
{
	// Add a bunch of asteroids.
	////////////////////////////
	SpawnAsteroids();
}