/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#ifndef _GT2AMATH_H_
#define _GT2AMATH_H_

#include <stdlib.h>
#include "gt2aMain.h"

byte * CopyV3b
(
	V3b dest,
	const V3b src
);

byte * SetV3b
(
	V3b dest,
	byte src0,
	byte src1,
	byte src2
);

float * CopyV2f
(
	V2f dest,
	const V2f src
);

float * SetV2f
(
	V2f dest,
	float src0,
	float src1
);

float * ScaleV2f
(
	V2f dest,
	const V2f src,
	float scale
);

float * AddV2f
(
	V2f dest,
	const V2f src0,
	const V2f src1
);

float * SubV2f
(
	V2f dest,
	const V2f src0,
	const V2f src1
);

float * SubScalarV2f
(
	V2f dest,
	const V2f src,
	float scalar
);

float LenV2f
(
	const V2f src
);

float DistanceV2f
(
	const V2f src0,
	const V2f src1
);

float * ClampV2f
(
	V2f dest,
	const V2f src,
	float min,
	float max
);

float ClampRotation
(
	float rotation
);

int ClampInt
(
	int num,
	int min,
	int max
);

float * RotationToVector
(
	V2f dest,
	float rotation
);

float * ComputeNewPosition
(
	V2f dest,
	const V2f position,
	int motion,
	float rotation,
	unsigned long diff,
	int unitsPerMilliSec,
	GT2Bool clamp
);

float ComputeNewRotation
(
	float rotation,
	int turning,
	unsigned long diff,
	float degreesPerMilliSec
);

float RandomFloat
(
	float minFloat,
	float maxFloat,
	GT2Bool maxInclusive
);

int RandomInt
(
	int minInt,
	int maxInt
);

unsigned short RotationToUnsignedShort
(
	float rotation
);

float UnsignedShortToRotation
(
	unsigned short packedRotation
);

unsigned short PositionToUnsignedShort
(
	float position
);

float UnsignedShortToPosition
(
	unsigned short packedPosition
);

#endif