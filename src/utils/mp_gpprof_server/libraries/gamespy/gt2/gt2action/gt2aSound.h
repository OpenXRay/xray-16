/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#ifndef _GT2ASOUND_H_
#define _GT2ASOUND_H_

enum
{
	SOUND_EXPLOSION,
	SOUND_MINE,
	SOUND_DIE,
	SOUND_ROCKET,
	SOUND_PICKUP,
	NUM_SOUNDS
};

GT2Bool InitializeSound
(
	void
);

void CleanupSound
(
	void
);

void ToggleSound
(
	void
);

void PlaySoundEffect
(
	int sound
);

#endif