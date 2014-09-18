/*
======================================================================
scenscan.h

Typedefs and function prototypes for a scene database.
====================================================================== */

#include <lwserver.h>
#include <lwmeshes.h>

typedef struct st_DBVMapRec {
   char        *name;
   LWID         type;
   int          dim;
} DBVMapRec;

typedef struct st_VertMapDB {
   int          nvmaps;
   DBVMapRec   *vmap;
} VertMapDB;


void freeVertMapDB( VertMapDB *vmdb );
VertMapDB *getVertMapDB( GlobalFunc *global );
