/*
gpiSearch.h
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifndef _GPISEARCH_H_
#define _GPISEARCH_H_

//INCLUDES
//////////
#include "gpi.h"

//TYPES
///////
#define GPI_SEARCH_PROFILE			    1
#define GPI_SEARCH_IS_VALID             2
#define GPI_SEARCH_NICKS                3
#define GPI_SEARCH_PLAYERS              4
#define GPI_SEARCH_CHECK                5
#define GPI_SEARCH_NEWUSER              6
#define GPI_SEARCH_OTHERS_BUDDY         7
#define GPI_SEARCH_SUGGEST_UNIQUE       8
#define GPI_SEARCH_OTHERS_BUDDY_LIST    9
#define GPI_SEARCH_PROFILE_UNIQUENICK  10

// A timeout used to abort searches taking too long
#define GPI_SEARCH_TIMEOUT 60000

// Profile Search operation data.
/////////////////////////////////
typedef struct
{
	int type;
	SOCKET sock;
	GPIBuffer inputBuffer;
	GPIBuffer outputBuffer;
	char nick[GP_NICK_LEN];
	char uniquenick[GP_UNIQUENICK_LEN];
	int namespaceIDs[GP_MAX_NAMESPACEIDS];
	int numNamespaces;
	char email[GP_EMAIL_LEN];
	char firstname[GP_FIRSTNAME_LEN];
	char lastname[GP_LASTNAME_LEN];
	char password[GP_PASSWORD_LEN];
	char cdkey[GP_CDKEY_LEN];
	int partnerID;
	int icquin;
	int skip;
	int productID;
	GPIBool processing;
	GPIBool remove;
	gsi_time searchStartTime;
	int *revBuddyProfileIds;
	int numOfRevBuddyProfiles;
} GPISearchData;

//FUNCTIONS
///////////
GPResult
gpiProfileSearch(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char firstname[GP_FIRSTNAME_LEN],
  const char lastname[GP_LASTNAME_LEN],
  int icquin,
  int skip,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult
gpiProfileSearchUniquenick(
  GPConnection * connection,
  const char uniquenick[GP_UNIQUENICK_LEN],
  const int namespaceIDs[],
  int numNamespaces,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult
gpiIsValidEmail(
  GPConnection * connection,
  const char email[GP_EMAIL_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult
gpiGetUserNicks(
  GPConnection * connection,
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult
gpiFindPlayers(
  GPConnection * connection,
  int productID,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult gpiCheckUser(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult gpiNewUser(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  const char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult gpiOthersBuddy(
  GPConnection * connection,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult gpiOthersBuddyList(
	GPConnection * connection,
	int *profiles, 
	int numOfProfiles,
	GPEnum blocking,
	GPCallback callback,
	void * param
);

GPResult gpiSuggestUniqueNick(
  GPConnection * connection,
  const char desirednick[GP_NICK_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

GPResult
gpiProcessSearches(
  GPConnection * connection
);

#endif
