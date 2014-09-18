/*
gpiUnique.h
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifndef _GPIUNIQUE_H_
#define _GPIUNIQUE_H_

//INCLUDES
//////////
#include "gpi.h"

//FUNCTIONS
///////////
GPResult gpiRegisterUniqueNick(
  GPConnection * connection,
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult gpiProcessRegisterUniqueNick(
  GPConnection * connection,
  GPIOperation * operation,
  const char * input
);

// Seperated registration of unique nick and cdkey
GPResult gpiRegisterCdKey(
  GPConnection * connection,
  const char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult gpiProcessRegisterCdKey(
  GPConnection * connection,
  GPIOperation * operation,
  const char * input
);

#endif
