/*
 * LWSDK Library Source File
 * Copyright 1995,1997  NewTek, Inc.
 *
 * Default 'Startup' function returns any non-zero value for success.
 */
#include <lwserver.h>


    void *
Startup (void)
{
    return (void *) 4;
}
