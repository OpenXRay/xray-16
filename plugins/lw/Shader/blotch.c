/*
======================================================================
blotch.c

Stick a colored spot on a surface.

Allen Hastings, Arnie Cachelin, Stuart Ferguson, Ernie Wright
6 April 00

** Note:  The first release of LW 6.0 will crash on exit if this
plug-in's interface has been opened during that session.  The crash
is related to xpanel destroy processing.  It shouldn't cause loss of
data, and it should be resolved in later builds.
====================================================================== */

#include <lwserver.h>
#include <lwshader.h>
#include <lwsurf.h>
#include <lwhost.h>
#include <lwxpanel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "LW_Shader.h"

#pragma warning (disable:4996)

#ifndef PI
#define PI 3.1415926535897932384
#endif
 
EShaderList ENShaders;
EShaderList LCShaders;
EShaderList GameMtls;

char* ENList_GetName(int idx){ 
	return ((idx>=0)&&(idx<ENShaders.count))?ENShaders.Names[idx]:"(none)";
}
int	  ENList_GetIdx(sh_name n){
	int k;
	for (k=0; k<ENShaders.count; k++){
		if (strcmp(n,ENShaders.Names[k])==0) return k;
	}
	return -1;
}
void  ENList_Clear(){
	ENShaders.count=0;
}


char* LCList_GetName(int idx){ 
	return ((idx>=0)&&(idx<LCShaders.count))?LCShaders.Names[idx]:"(none)";
}
int	  LCList_GetIdx(sh_name n){
	int k;
	for (k=0; k<LCShaders.count; k++){
		if (strcmp(n,LCShaders.Names[k])==0) return k;
	}
	return -1;
}
void  LCList_Clear(){
	LCShaders.count=0;
}

char* GMList_GetName(int idx){ 
	return ((idx>=0)&&(idx<GameMtls.count))?GameMtls.Names[idx]:"(none)";
}
int	  GMList_GetIdx(sh_name n){
	int k;
	for (k=0; k<GameMtls.count; k++){
		if (strcmp(n,GameMtls.Names[k])==0) return k;
	}
	return -1;
}
void  GMList_Clear(){
	GameMtls.count=0;
}
/*
======================================================================
popCnt_VMAP()

Return the number of weight maps, plus 1 for "(none)".  An xpanel
callback for the vmap popup list, also used by Load().
====================================================================== */

XCALL_( static int )
popCnt_EN( void *data )
{
	return 1 + ENShaders.count;
}

XCALL_( static int )
popCnt_GM( void *data )
{
	return 1 + GameMtls.count;
}

XCALL_( static int )
popCnt_LC( void *data )
{
	return 1 + LCShaders.count;
}

/*
======================================================================
popName_VMAP()

Return the name of a vmap, given an index.  An xpanel callback for the
vmap popup list, also used by Load() and Save().
====================================================================== */

XCALL_( static const char * )
popName_EN( void *data, int idx )
{
	return ENList_GetName(idx);
}

XCALL_( static const char * )
popName_GM( void *data, int idx )
{
	return GMList_GetName(idx);
}

XCALL_( static const char * )
popName_LC( void *data, int idx )
{
	return LCList_GetName(idx);
}

/*
======================================================================
Create()

Handler callback.  Allocate and initialize instance data.

The create function allocates a blotch struct and returns the pointer
as the instance.  Note that "Blotch *" is used throughout instead of
"LWInstance".  This works since a LWInstance type is a generic pointer
and can safely be replaced with any specific pointer type.  Instance
variables are initialized to some default values.
====================================================================== */
void __cdecl LoadShaders();


XCALL_( static LWInstance )
Create( void *priv, LWSurfaceID surf, LWError *err )
{
   XRShader *inst;

   inst = calloc( 1, sizeof( XRShader ));
   if ( !inst ) {
      *err = "Couldn't allocate memory for instance.";
      return NULL;
   }

   strcpy(inst->en_name, "default");
   strcpy(inst->lc_name, "default");
   strcpy(inst->gm_name, "default");

   ENList_Clear();
   LCList_Clear();
   LoadShaders();

   return inst;
}


/*
======================================================================
Destroy()

Handler callback.  Free resources allocated by Create().
====================================================================== */

XCALL_( static void )
Destroy( XRShader *inst )
{
   free( inst );
}


/*
======================================================================
Copy()

Handler callback.  Copy instance data.
====================================================================== */

XCALL_( static LWError )
Copy( XRShader *to, XRShader *from )
{
   XCALL_INIT;

   *to = *from;
   return NULL;
}


/*
======================================================================
Load()

Handler callback.  Read instance data.  Shader instance data is stored
in the SURF chunks of object files, but it isn't necessary to know
that to read and write the data.
====================================================================== */

XCALL_( static LWError )
Load( XRShader *inst, const LWLoadState *ls )
{
	ls->read(ls->readData,(char*)inst,sizeof(XRShader));
	if (ENList_GetIdx(inst->en_name)==-1) strcpy(inst->en_name,"default");
	if (LCList_GetIdx(inst->lc_name)==-1) strcpy(inst->lc_name,"default");
	if (GMList_GetIdx(inst->gm_name)==-1) strcpy(inst->gm_name,"default");
	inst->desc = 0;

	return NULL;
}


/*
======================================================================
Save()

Handler callback.  Write instance data.  The I/O functions in lwio.h
include one for reading and writing floats, but not doubles.  We just
transfer our double-precision data to a float variable before calling
the LWSAVE_FP() macro.
====================================================================== */

XCALL_( static LWError )
Save( XRShader *inst, const LWSaveState *ss )
{
	ss->write(ss->writeData,(char*)inst,sizeof(XRShader));

	return NULL;
}


/*
======================================================================
DescLn()

Handler callback.  Write a one-line text description of the instance
data.  Since the string must persist after this is called, it's part
of the instance.
====================================================================== */

XCALL_( static const char * )
DescLn( XRShader *inst )
{
	char s[1024];
	if (inst->desc) free(inst->desc);
	sprintf( s, "ES:'%s', CS:'%s', GM:'%s'", inst->en_name, inst->lc_name, inst->gm_name );
	inst->desc = strdup(s);
	return inst->desc;
}


/*
======================================================================
Init()

Handler callback, called at the start of rendering.  We do a little
precalculation here.
====================================================================== */

XCALL_( static LWError )
Init( XRShader *inst, int mode )
{
	inst->desc = 0;
   return NULL;
}


/*
======================================================================
Cleanup()

Handler callback, called at the end of rendering.  We don't have
anything to do, but it's here in case we want to add something later.
====================================================================== */

XCALL_( static void )
Cleanup( XRShader *inst )
{
	if (inst->desc) free(inst->desc);
	return;
}


/*
======================================================================
NewTime()

Handler callback, called at the start of each sampling pass.
====================================================================== */

XCALL_( static LWError )
NewTime( XRShader *inst, LWFrame f, LWTime t )
{
   return NULL;
}


/*
======================================================================
Flags()

Handler callback.  Blotch alters the color of the surface, but nothing
else, so we return just the color bit.
====================================================================== */

XCALL_( static unsigned int )
Flags( XRShader *inst )
{
   return LWSHF_COLOR;
}


/*
======================================================================
Evaluate()

Handler callback.  This is where the blotchiness actually happens.  We
compute the distance from the spot to be shaded to the center of the
blotch and blend some of the blotch color with the color already
computed for that spot.
====================================================================== */

XCALL_( static void )
Evaluate( XRShader *inst, LWShaderAccess *sa )
{
}


/*
======================================================================
Handler()

Handler activation function.  Check the version and fill in the
callback fields of the handler structure.
====================================================================== */

XCALL_( static int )
Handler( long version, GlobalFunc *global, LWShaderHandler *local,
   void *serverData)
{
   if ( version != LWSHADER_VERSION ) return AFUNC_BADVERSION;

   local->inst->create   = Create;
   local->inst->destroy  = Destroy;
   local->inst->load     = Load;
   local->inst->save     = Save;
   local->inst->copy     = Copy;
   local->inst->descln   = DescLn;
   local->rend->init     = Init;
   local->rend->cleanup  = Cleanup;
   local->rend->newTime  = NewTime;
   local->evaluate       = Evaluate;
   local->flags          = Flags;

   return AFUNC_OK;
}



/* interface stuff ----- */

static LWXPanelFuncs *xpanf;
static LWColorActivateFunc *colorpick;
static LWInstUpdate *lwupdate;

enum {	ID_EN = 0x8001,
		ID_LC = 0x8002,
		ID_GM = 0x8003};

/*
======================================================================
ui_get()

Xpanels callback for LWXP_VIEW panels.  Returns a pointer to the data
for a given control value.
====================================================================== */
void *ui_get( XRShader *dat, unsigned long vid )
{
	void *result = NULL;

	if ( dat )
		switch ( vid ) {
		case ID_EN:
			dat->en_idx = ENList_GetIdx(dat->en_name);
			result = &dat->en_idx;
		break;
		case ID_LC:
			dat->lc_idx = LCList_GetIdx(dat->lc_name);
			result = &dat->lc_idx;
		break;
		case ID_GM:
			dat->gm_idx = GMList_GetIdx(dat->gm_name);
			result = &dat->gm_idx;
			break;
		}

   return result;
}


/*
======================================================================
ui_set()

Xpanels callback for LWXP_VIEW panels.  Store a value in our instance
data.
====================================================================== */

LWXPRefreshCode ui_set( XRShader *dat, unsigned long vid, void *value )
{
	switch ( vid ) {
		case ID_EN:
			dat->en_idx = *(( int * ) value );
			if(dat->en_idx>=0) strncpy(dat->en_name, ENList_GetName(dat->en_idx) ,sizeof( dat->en_name ));
		break;
		case ID_LC:
			dat->lc_idx = *(( int * ) value );
			if(dat->lc_idx>=0) strncpy(dat->lc_name, LCList_GetName(dat->lc_idx) ,sizeof( dat->lc_name ));
		break;
		case ID_GM:
			dat->gm_idx = *(( int * ) value );
			if(dat->gm_idx>=0) strncpy(dat->gm_name, GMList_GetName(dat->gm_idx) ,sizeof( dat->gm_name ));
			break;
		default:
			return 0;
	}
	return 1;
}


/*
======================================================================
ui_chgnotify()

XPanel callback.  XPanels calls this when an event occurs that affects
the value of one of your controls.  We use the instance update global
to tell Layout that our instance data has changed.
====================================================================== */

void ui_chgnotify( LWXPanelID panel, unsigned long cid, unsigned long vid,
   int event )
{
   void *dat;

   if ( event == LWXPEVENT_VALUE )
      if ( dat = xpanf->getData( panel, 0 ))
         lwupdate( LWSHADER_HCLASS, dat );
}


/*
======================================================================
get_panel()

Create and initialize an LWXP_VIEW panel.  Called by Interface().
====================================================================== */
#define STR_Type_EN  "Engine"
#define STR_Type_LC  "Compiler"
#define STR_Type_GM  "Game Material"
static LWXPanelControl ctrl_list[] = {
   { ID_EN, STR_Type_EN,  "iPopChoice" },
   { ID_LC, STR_Type_LC,  "iPopChoice" },
   { ID_GM, STR_Type_GM,  "iPopChoice" },
   { 0 }
};

/* matching array of data descriptors */

static LWXPanelDataDesc data_descrip[] = {
   { ID_EN, STR_Type_EN,  "integer"   },
   { ID_LC, STR_Type_LC,  "integer"   },
   { ID_GM, STR_Type_GM,  "integer"   },
   { 0 },
};
static LWXPanelID get_xpanel( GlobalFunc *global, XRShader *dat )
{
   LWXPanelID panID = NULL;

   static LWXPanelHint hint[] = {
      XpLABEL( 0, "Shaders" ),
      XpCHGNOTIFY( ui_chgnotify ),
      XpPOPFUNCS( ID_EN, popCnt_EN, popName_EN ),
      XpPOPFUNCS( ID_LC, popCnt_LC, popName_LC ),
	  XpPOPFUNCS( ID_GM, popCnt_GM, popName_GM ),
      XpEND
   };

   xpanf = global( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT );
   if ( xpanf ) {
      panID = xpanf->create( LWXP_VIEW, ctrl_list );
      if ( panID ) {
         xpanf->hint( panID, 0, hint );
         xpanf->describe( panID, data_descrip, ui_get, ui_set );
         xpanf->viewInst( panID, dat );
         xpanf->setData( panID, 0, dat );
      }
   }

   return panID;
}

/*
======================================================================
Interface()

The interface activation function.
====================================================================== */

XCALL_( int )
Interface( long version, GlobalFunc *global, LWInterface *local,
   void *serverData )
{
   if ( version != LWINTERFACE_VERSION ) return AFUNC_BADVERSION;

   colorpick = global( LWCOLORACTIVATEFUNC_GLOBAL, GFUSE_TRANSIENT );
   lwupdate  = global( LWINSTUPDATE_GLOBAL,        GFUSE_TRANSIENT );
   xpanf     = global( LWXPANELFUNCS_GLOBAL,       GFUSE_TRANSIENT );
   if ( !colorpick || !lwupdate || !xpanf ) return AFUNC_BADGLOBAL;

   local->panel	  = get_xpanel( global, local->inst );
//   local->panel   = get_panel( local->inst );
   local->options = NULL;
   local->command = NULL;

   return local->panel ? AFUNC_OK : AFUNC_BADGLOBAL;
}

ServerRecord ServerDesc[] = {
   { LWSHADER_HCLASS, SH_PLUGIN_NAME, Handler },
   { LWSHADER_ICLASS, SH_PLUGIN_NAME, Interface },
   { NULL }
};

#pragma warning (default:4996)
