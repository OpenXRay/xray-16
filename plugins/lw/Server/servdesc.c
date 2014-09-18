/*
 * LWSDK Library Source File
 * Copyright 1995,1997  NewTek, Inc.
 *
 * Default 'ServerDesc' array is defined by assuming the existence of
 * some other static globals for the class name, server name, user
 * names and activation entry point for a single server.  Multiple
 * server modules will define their own 'ServerDesc' arrays.
 */
#include <lwserver.h>


extern char             ServerClass[];
extern char             ServerName[];
extern ServerUserName   UserNames[];
extern XCALL_(int)    Activate (long, GlobalFunc *, void *, void *);


    ServerRecord
ServerDesc[] = {
    { ServerClass, ServerName, Activate, UserNames },
    { 0 }
};

