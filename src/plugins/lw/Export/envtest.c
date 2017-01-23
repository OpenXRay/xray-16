/*
======================================================================
envtest.c

A plug-in to test the envelope interpolation routine in interp.c.
  
Ernie Wright  31 Aug 00
	
This is what I used to test evalEnvelope().  It reads the Position.X
envelope for the camera, then compares what evalEnvelope() returns to
what LWEnvelopeFuncs->evaluate() says.
====================================================================== */

#include <lwserver.h>
#include <lwgeneric.h>
#include <lwrender.h>
#include <lwenvel.h>
#include <lwchannel.h>
#include <lwhost.h>
#include <lwpanel.h>
#include <lwsurf.h>
#include <lwdisplay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* some globals */

LWChannelInfo	*g_chinfo;
LWEnvelopeFuncs *g_envf;
LWItemInfo		*g_iteminfo;
LWSceneInfo		*g_lwsi;
LWInterfaceInfo *g_intinfo;
LWMessageFuncs	*g_msg;
LWBoneInfo		*g_boneinfo;
LWObjectFuncs	*g_objfunc;
LWObjectInfo	*g_objinfo;
LWSurfaceFuncs	*g_surff;
HostDisplayInfo *g_hdi;
	

static int get_globals( GlobalFunc *global )
{
	g_chinfo	= global( LWCHANNELINFO_GLOBAL,		GFUSE_TRANSIENT );
	g_envf		= global( LWENVELOPEFUNCS_GLOBAL,	GFUSE_TRANSIENT );
	g_iteminfo	= global( LWITEMINFO_GLOBAL,		GFUSE_TRANSIENT );
	g_lwsi		= global( LWSCENEINFO_GLOBAL,		GFUSE_TRANSIENT );
	g_intinfo	= global( LWINTERFACEINFO_GLOBAL,	GFUSE_TRANSIENT );
	g_msg		= global( LWMESSAGEFUNCS_GLOBAL,	GFUSE_TRANSIENT );
	g_boneinfo	= global( LWBONEINFO_GLOBAL,		GFUSE_TRANSIENT );	
	g_objfunc	= global( LWOBJECTFUNCS_GLOBAL,		GFUSE_TRANSIENT );
	g_objinfo	= global( LWOBJECTINFO_GLOBAL,		GFUSE_TRANSIENT );
	g_surff		= global( LWSURFACEFUNCS_GLOBAL,	GFUSE_TRANSIENT );
	g_hdi		= global( LWHOSTDISPLAYINFO_GLOBAL, GFUSE_TRANSIENT );
	
	return ( g_chinfo && g_envf && g_iteminfo && g_lwsi && g_intinfo && g_msg && g_boneinfo && g_objfunc && g_objinfo && g_surff);
}


//======================================================================
//KeyExport()
//The activation function.
//======================================================================
void __cdecl SaveSkeletonMotion(GlobalFunc *global);
XCALL_( int )SkeletonMotionExport( long version, GlobalFunc *global, LWLayoutGeneric *local, void *serverData ){
	if (version!=LWLAYOUTGENERIC_VERSION) return AFUNC_BADVERSION;
	if (!get_globals(global)) return AFUNC_BADGLOBAL;
	
	SaveSkeletonMotion(global);
	
	return AFUNC_OK;
}
//-----------------------------------------------------------------------------------------

//======================================================================
//KeyExport()
//The activation function.
//======================================================================
void __cdecl SaveObject(GlobalFunc *global);
XCALL_( int )ObjectExport( long version, GlobalFunc *global, LWLayoutGeneric *local, void *serverData ){
	if (version!=LWLAYOUTGENERIC_VERSION) return AFUNC_BADVERSION;
	if (!get_globals(global)) return AFUNC_BADGLOBAL;
	
	SaveObject(global);
	
	return AFUNC_OK;
}
//-----------------------------------------------------------------------------------------

//======================================================================
//KeyExport()
//The activation function.
//======================================================================
void __cdecl SaveObjectMotion(GlobalFunc *global);
XCALL_( int )ObjectMotionExport( long version, GlobalFunc *global, LWLayoutGeneric *local, void *serverData ){
	if (version!=LWLAYOUTGENERIC_VERSION) return AFUNC_BADVERSION;
	if (!get_globals(global)) return AFUNC_BADGLOBAL;
	
	SaveObjectMotion(global);
	
	return AFUNC_OK;
}

ServerRecord ServerDesc[] = {
	{ LWLAYOUTGENERIC_CLASS, "XRay_Skeleton_Motion_Export",	SkeletonMotionExport },
	{ LWLAYOUTGENERIC_CLASS, "XRay_Object_Export",			ObjectExport },
	{ LWLAYOUTGENERIC_CLASS, "XRay_Object_Motion_Export",	ObjectMotionExport },
	{ NULL }
};
