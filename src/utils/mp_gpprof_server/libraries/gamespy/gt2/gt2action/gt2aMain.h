/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#ifndef _GT2AMAIN_H_
#define _GT2AMAIN_H_

#include <stdio.h>
#include <GL/glut.h>
#include "../../nonport.h"
#include "../gt2.h"
#include "../gt2Encode.h"

#define PORT_STRING          ":12345"

#define MAX_PLAYERS         64

#define BACKWARD            -1
#define STILL                0
#define FORWARD              1

#define LEFT                -1
#define RIGHT                1

#define MAP_MIN              0
#define MAP_MAX         100000.0
#define MAP_HALF             (MAP_MAX / 2)
#define MAP_EXTRA            (MAP_MAX / 10)

#define MAX_NICK            32

#define CHAT_MAX            64

#define MSG_C_INITIAL        1
#define MSG_C_INITIAL_STR    "s"
#define MSG_C_UPDATE         2
#define MSG_C_UPDATE_STR     "ppzzzz"
#define MSG_C_PRESS          3
#define MSG_C_PRESS_STR      "s"
#define MSG_C_CHAT           4
#define MSG_C_CHAT_STR       "s"
#define MSG_S_ADDCLIENT      1001
#define MSG_S_ADDCLIENT_STR  "bs"
#define MSG_S_DELCLIENT      1002
#define MSG_S_DELCLIENT_STR  "b"
#define MSG_S_START          1003
#define MSG_S_START_STR      "b"
#define MSG_S_UPDATE         1004
#define MSG_S_UPDATE_STR     "pbb"
#define MSG_S_UPDATE_CLIENT_STR    "bpppizzzzz"
#define MSG_S_UPDATE_OBJECT_STR    "bpppi"
#define MSG_S_CHAT           1005
#define MSG_S_CHAT_STR       "s"
#define MSG_S_SOUND          1006
#define MSG_S_SOUND_STR      "b"
#define MSG_S_NUMASTEROIDS   1007
#define MSG_S_NUMASTEROIDS_STR      "b"

typedef unsigned char byte;
typedef float V2f[2];
typedef byte V3b[3];

extern const V3b Red;
extern const V3b Green;
extern const V3b Blue;
extern const V3b Yellow;
extern const V3b Orange;
extern const V3b Purple;
extern const V3b Black;
extern const V3b White;
extern const V3b Grey;

void Log
(
	const char * format,
	...
);

#endif