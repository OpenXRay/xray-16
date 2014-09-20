/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#ifndef _GT2ADISPLAY_H_
#define _GT2ADISPLAY_H_

extern GT2Bool fullScreen;

extern GT2Bool DrawGraphsOption;
extern GT2Bool DrawUpdateTimeOption;
extern GT2Bool DrawUpdateLengthOption;
extern GT2Bool DrawFrameTimeOption;
extern GT2Bool DrawMarksOption;
extern GT2Bool DrawFPSOption;
extern GT2Bool DrawWithPredictionOption;
extern GT2Bool DrawBackgroundOption;
extern GT2Bool DrawStarsOption;
extern GT2Bool DrawBoundingCirclesOption;
extern GT2Bool DrawRadarOption;
extern float RadarScaleOption;
extern float RadarPointScaleOption;
extern GT2Bool ViewClippingOption;

extern int screenWidth;
extern int screenHeight;

void InitializeDisplay
(
	void
);

void ShutdownDisplay
(
	void
);

void DisplayThink
(
	unsigned long now
);

void DisplayChat
(
	const char * message
);

#endif