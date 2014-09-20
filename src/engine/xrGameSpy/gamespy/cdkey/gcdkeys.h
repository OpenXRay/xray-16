/******
gcdkeys.h
GameSpy CDKey SDK Server Header
  
Copyright 1999-2001 GameSpy Industries, Inc

18002 Skypark Circle
Irvine, California 92614
949.798.4200 (Tel)
949.798.4299 (Fax)
devsupport@gamespy.com

******

 Please see the GameSpy CDKey SDK documentation for more 
 information

******/


#ifndef _GOACDKEYS_H_
#define _GOACDKEYS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GUSE_ASSERTS

/*****
QR2CDKEY_INTEGRATION: This define controls where the functions needed to integrate
the networking of the Query & Reporting 2 SDK and CDKey SDKs are available.
If you intend to use the integration option for these SDKs, you must uncomment the 
define below, or provide it as an option to your compiler.
*******/
#define QR2CDKEY_INTEGRATION

typedef void (*AuthCallBackFn)(int gameid, int localid, int authenticated, char *errmsg, void *instance);
typedef void (*RefreshAuthCallBackFn)(int gameid, int localid, int hint, char *challenge, void *instance);

/* The hostname of the validation server.
If the app resolves the hostname, an
IP can be stored here before calling
gcd_init */
extern char gcd_hostname[64];

/********
gcd_init
Initializes the Server API and creates the socket
Should only be called once (unless gcd_shutdown has been called)
*********/
int gcd_init(int gameid);

/********
gcd_init_qr2
Initializes the Server API and integrates the networking of the CDKey SDK
with the Query & Reporting 2 SDK.
You must initialize the Query & Reporting 2 SDK with qr2_init or qr2_init_socket
prior to calling this. If you are using multiple instances of the QR2 SDK, you
can pass the specific instance information in via the "qrec" argument. Otherwise
you can simply pass in NULL.
*********/
#ifdef QR2CDKEY_INTEGRATION

#include "../qr2/qr2.h"
int gcd_init_qr2(qr2_t qrec, int gameid);

#endif

/********
gcd_shutdown
Frees the socket and client structures
Also calls gcd_disconnect_all to make sure all users are signaled as offline
*********/
void gcd_shutdown(void);

/********
gcd_authenticate_user
Creates a new client and sends a request for authorization to the
validation server.
*********/
void gcd_authenticate_user(int gameid, int localid, unsigned int userip, const char *challenge, 
						   const char *response, AuthCallBackFn authfn, RefreshAuthCallBackFn refreshfn, void *instance);

/********
gcd_authenticate_user
Creates a new client and sends a request for authorization to the
validation server.
*********/
void gcd_process_reauth(int gameid, int localid, int hint, const char *response);


/********
gcd_disconnect_user
Notify the validation server that a user has disconnected
*********/
void gcd_disconnect_user(int gameid, int localid);


/********
gcd_disconnect_all
Calls gcd_disconnect_user for each user still online (shortcut)
*********/
void gcd_disconnect_all(int gameid);

/********
gcd_think
Processes any pending data from the validation server
and calls the callback to indicate whether a client was 
authorized or not
*********/
void gcd_think(void);

/********
gcd_getkeyhash
Returns the key hash for the given user. This hash will always
be the same for that users, which makes it good for banning or
tracking of users (used with the Tracking/Stats SDK). Returns
an empty string if that user isn't connected.
*********/
char *gcd_getkeyhash(int gameid, int localid);

#ifdef __cplusplus
}
#endif

#endif

