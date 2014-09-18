/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include <math.h>
#include <float.h>
#include <limits.h>
#include "gt2aMain.h"
#include "gt2aMath.h"

#define PI                3.1415926535897932384626433832795
#define PI_DIV_180        (PI / 180)

byte * CopyV3b
(
	V3b dest,
	const V3b src
)
{
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];

	return dest;
}

byte * SetV3b
(
	V3b dest,
	byte src0,
	byte src1,
	byte src2
)
{
	dest[0] = src0;
	dest[1] = src1;
	dest[2] = src2;

	return dest;
}

float * CopyV2f
(
	V2f dest,
	const V2f src
)
{
	dest[0] = src[0];
	dest[1] = src[1];

	return dest;
}

float * SetV2f
(
	V2f dest,
	float src0,
	float src1
)
{
	dest[0] = src0;
	dest[1] = src1;

	return dest;
}

float * ScaleV2f
(
	V2f dest,
	const V2f src,
	float scale
)
{
	dest[0] = (src[0] * scale);
	dest[1] = (src[1] * scale);

	return dest;
}

float * AddV2f
(
	V2f dest,
	const V2f src0,
	const V2f src1
)
{
	dest[0] = (src0[0] + src1[0]);
	dest[1] = (src0[1] + src1[1]);

	return dest;
}

float * SubV2f
(
	V2f dest,
	const V2f src0,
	const V2f src1
)
{
	dest[0] = (src0[0] - src1[0]);
	dest[1] = (src0[1] - src1[1]);

	return dest;
}

float * SubScalarV2f
(
	V2f dest,
	const V2f src,
	float scalar
)
{
	dest[0] = (src[0] - scalar);
	dest[1] = (src[1] - scalar);

	return dest;
}

float LenV2f
(
	const V2f src
)
{
	return (float)sqrt((src[0] * src[0]) + (src[1] * src[1]));
}

float DistanceV2f
(
	const V2f src0,
	const V2f src1
)
{
	V2f temp;

	SubV2f(temp, src0, src1);

	return LenV2f(temp);
}

float * ClampV2f
(
	V2f dest,
	const V2f src,
	float min,
	float max
)
{
	float temp;
	int i;

	for(i = 0 ; i < 2 ; i++)
	{
		temp = src[i];
		if(temp > max)
			dest[i] = max;
		else if(temp < min)
			dest[i] = min;
		else
			dest[i] = temp;
	}

	return dest;
}

float ClampRotation
(
	float rotation
)
{
	while(rotation >= 360)
		rotation -= 360;
	while(rotation < 0)
		rotation += 360;
	return rotation;
}

int ClampInt
(
	int num,
	int min,
	int max
)
{
	if(num < min)
		num = min;
	else if(num > max)
		num = max;
	return num;
}

float * RotationToVector
(
	V2f dest,
	float rotation
)
{
	// Convert to radians.
	//////////////////////
	rotation = (float)(rotation * PI_DIV_180);

	dest[0] = (float)sin(rotation);
	dest[1] = (float)cos(rotation);

	return dest;
}

float * ComputeNewPosition
(
	V2f dest,
	const V2f position,
	int motion,
	float rotation,
	unsigned long diff,
	int unitsPerSec,
	GT2Bool clamp
)
{
	V2f forward;
	float scale;

	// Check for no motion.
	///////////////////////
	if(!motion)
	{
		CopyV2f(dest, position);
		return dest;
	}

	// Get the direction we're facing.
	//////////////////////////////////
	RotationToVector(forward, rotation);

	// Scale it based on the amount of time.
	////////////////////////////////////////
	scale = (float)(diff * unitsPerSec / 1000);
	if(motion == BACKWARD)
		scale = -scale;
	ScaleV2f(forward, forward, scale);

	// Update the position.
	///////////////////////
	AddV2f(dest, position, forward);

	// Clamp it.
	////////////
	if(clamp)
		ClampV2f(dest, dest, 0, MAP_MAX);

	return dest;
}

float ComputeNewRotation
(
	float rotation,
	int turning,
	unsigned long diff,
	float degreesPerMilliSec
)
{
	// Turn.
	////////
	rotation += (turning * (diff * degreesPerMilliSec));

	// Clamp it to between 0 and 360.
	/////////////////////////////////
	while(rotation >= 360)
		rotation -= 360;
	while(rotation < 0)
		rotation += 360;

	return rotation;
}

float RandomFloat
(
	float minFloat,
	float maxFloat,
	GT2Bool maxInclusive
)
{
	float num;

	num = rand();
	num *= (maxFloat - minFloat);
	num /= (RAND_MAX + (maxInclusive?0:1));
	num += minFloat;

	return num;
}

int RandomInt
(
	int minInt,
	int maxInt
)
{
	return (minInt + ((rand() * (maxInt - minInt + 1)) / (RAND_MAX + 1)));
}

unsigned short RotationToUnsignedShort
(
	float rotation
)
{
	rotation *= 0xFFFF;
	rotation /= 360;
	if(rotation < 0)
		rotation = 0;
	if(rotation > 0xFFFF)
		rotation = 0xFFFF;

	return (unsigned short)rotation;
}

float UnsignedShortToRotation
(
	unsigned short packedRotation
)
{
	float rotation;

	rotation = packedRotation;
	rotation *= 360;
	rotation /= 0xFFFF;

	return ClampRotation(rotation);
}

unsigned short PositionToUnsignedShort
(
	float position
)
{
	position += (MAP_EXTRA / 2);
	position *= 0xFFFF;
	position /= (MAP_MAX + MAP_EXTRA);
	if(position < 0)
		position = 0;
	else if(position > USHRT_MAX)
		position = USHRT_MAX;

	return (unsigned short)position;
}

float UnsignedShortToPosition
(
	unsigned short packedPosition
)
{
	float position;

	position = packedPosition;
	position *= (MAP_MAX + MAP_EXTRA);
	position /= 0xFFFF;
	position -= (MAP_EXTRA / 2);

	return position;
}