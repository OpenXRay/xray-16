/*
======================================================================
envelope.h

Definitions for local copies of LightWave envelopes.

Ernie Wright  31 Aug 00

The LightWave plug-in SDK provides its own representation of LightWave
envelopes that plug-ins can use.  These definitions are for standalone
programs that, for example, read scene or object files and must store
the envelopes.
====================================================================== */

#define SHAPE_TCB   0
#define SHAPE_HERM  1
#define SHAPE_BEZI  2
#define SHAPE_LINE  3
#define SHAPE_STEP  4
#define SHAPE_BEZ2  5

#define BEH_RESET      0
#define BEH_CONSTANT   1
#define BEH_REPEAT     2
#define BEH_OSCILLATE  3
#define BEH_OFFSET     4
#define BEH_LINEAR     5


typedef struct st_Key {
   struct st_Key *next;
   struct st_Key *prev;
   float  value;
   float  time;
   int    shape;
   float  tension;
   float  continuity;
   float  bias;
   float  param[ 4 ];
} Key;

typedef struct st_Envelope {
   Key   *key;
   int    nkeys;
   int    behavior[ 2 ];
} Envelope;


float evalEnvelope( Envelope *env, float time );
