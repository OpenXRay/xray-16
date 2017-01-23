#include "stdafx.h"
/*
======================================================================
surface.c

  Functions for examining surfaces.
====================================================================== */
#include "scenscan.h"
#include "objectdb.h"
#include <lwhost.h>

#pragma warning (disable:4995)

extern "C"	LWMessageFuncs	*g_msg;
/*
======================================================================
findSurf()

  Linear search the surface array and return the array index with the
  matching surface name.
====================================================================== */

static int findSurf( ObjectDB *odb, const char *tag )
{
	int i;
	
	for ( i = 0; i < odb->nsurfaces; i++ )
		if ( !strcmp( tag, odb->surf[ i ].name ))
			return i;
		
		return -1;
}


/*
======================================================================
freeObjectSurfs()

  Free memory allocated by getObjectSurfs().
====================================================================== */

void freeObjectSurfs( ObjectDB *odb )
{
	int i;
	
	if ( odb ) {
		if ( odb->surf ) {
			for ( i = 0; i < odb->nsurfaces; i++ )
				if ( odb->surf[ i ].name )
					free( odb->surf[ i ].name );
				free( odb->surf );
				odb->surf = 0;
				odb->nsurfaces = 0;
		}
	}
}


/*
======================================================================
getObjectSurfs()

  Allocate and fill in structures describing the surfaces applied to the
  object.  Updates the ObjectDB and returns 1 if successful.  Backs out
  of any changes to the ObjectDB and returns 0 if an error occurs.
  
	Currently this isn't very sophisticated.  We just get the values.  The
	SDK also allows us to get the envelopes and textures for most of these
	parameters (anything with an E or a T button in the Surface Editor),
	so we could do much more here.  Given a channel's LWTextureID, for
	example, we could use the Texture Functions global to get each texture
	layer's parameters, including the image maps.
====================================================================== */

int getObjectSurfs( ObjectDB *odb, LWMeshInfo *mesh, GlobalFunc *global )
{
	LWSurfaceFuncs *surff;
	LWSurfaceID *surfid;
	LWTextureFuncs *txfunc;
	LWImageList *imglist;
   	double *dval;
	int i;
	const char* tag;
	
	
	imglist = (st_LWImageList*)global( LWIMAGELIST_GLOBAL, GFUSE_TRANSIENT );
	if ( !imglist ) return 0;
	txfunc = (st_LWTextureFuncs*)global( LWTEXTUREFUNCS_GLOBAL, GFUSE_TRANSIENT );   /* get the surface ID array */
	if ( !txfunc ) return 0;
	surff = (st_LWSurfaceFuncs*)global( LWSURFACEFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if ( !surff ) return 0;
	surfid = surff->byObject( odb->filename );
	if ( !surfid ) return 0;
	
	/* count the surface IDs and alloc the surface array */
	
	for ( odb->nsurfaces = 0; ; odb->nsurfaces++ )
		if ( !surfid[ odb->nsurfaces ] ) break;
		odb->surf = (st_DBSurface*)calloc( odb->nsurfaces, sizeof( DBSurface ));
		if ( !odb->surf ) {
			odb->nsurfaces = 0;
			return 0;
		}
		
		/* fill in the surface parameters */
		
		for ( i = 0; i < odb->nsurfaces; i++ ) {
			odb->surf[ i ].id = surfid[ i ];
			tag = surff->name( surfid[ i ] );
			odb->surf[ i ].name = (char*)malloc( xr_strlen( tag ) + 1 );
			if ( !odb->surf[ i ].name ) {
				freeObjectSurfs( odb );
				return 0;
			}
			strcpy( odb->surf[ i ].name, tag );
			
			LWTextureID tid = surff->getTex(surfid[i],SURF_COLR);	
			if (!tid){
				g_msg->error("Empty texture in surface:",odb->surf[ i ].name);
				freeObjectSurfs( odb );
				return 0;
			}
			for (LWTLayerID tlid=txfunc->firstLayer(tid); tlid; tlid=txfunc->nextLayer(tid,tlid)){
				DWORD imid;
				int res = txfunc->getParam(tlid,TXTAG_IMAGE,&imid);
				tag = imglist->name((LWImageID)imid);
				if (!tag){
					g_msg->error("Invalid texture name.",0);
					return 0;
				}
				strcpy(odb->surf[i].textures[odb->surf[i].tex_cnt],tag);
				odb->surf[i].tex_cnt++;
			}
			
			
			dval = surff->getFlt( surfid[ i ], SURF_COLR );
			odb->surf[ i ].colr[ 0 ] = ( float ) dval[ 0 ];
			odb->surf[ i ].colr[ 1 ] = ( float ) dval[ 1 ];
			odb->surf[ i ].colr[ 2 ] = ( float ) dval[ 2 ];
			
			dval = surff->getFlt( surfid[ i ], SURF_LUMI );
			odb->surf[ i ].lumi = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_DIFF );
			odb->surf[ i ].diff = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_SPEC );
			odb->surf[ i ].spec = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_REFL );
			odb->surf[ i ].refl = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_TRAN );
			odb->surf[ i ].tran = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_TRNL );
			odb->surf[ i ].trnl = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_RIND );
			odb->surf[ i ].rind = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_BUMP );
			odb->surf[ i ].bump = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_GLOS );
			odb->surf[ i ].glos = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_SHRP );
			odb->surf[ i ].shrp = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_SMAN );
			odb->surf[ i ].sman = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_RSAN );
			odb->surf[ i ].rsan = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_TSAN );
			odb->surf[ i ].tsan = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_CLRF );
			odb->surf[ i ].clrf = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_CLRH );
			odb->surf[ i ].clrh = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_ADTR );
			odb->surf[ i ].adtr = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_AVAL );
			odb->surf[ i ].aval = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_GVAL );
			odb->surf[ i ].gval = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_LSIZ );
			odb->surf[ i ].lsiz = ( float ) *dval;
			
			dval = surff->getFlt( surfid[ i ], SURF_LCOL );
			odb->surf[ i ].lcol[ 0 ] = ( float ) dval[ 0 ];
			odb->surf[ i ].lcol[ 1 ] = ( float ) dval[ 1 ];
			odb->surf[ i ].lcol[ 2 ] = ( float ) dval[ 2 ];
			
			odb->surf[ i ].alph = surff->getInt( surfid[ i ], SURF_ALPH );
			odb->surf[ i ].rfop = surff->getInt( surfid[ i ], SURF_RFOP );
			odb->surf[ i ].trop = surff->getInt( surfid[ i ], SURF_TROP );
			odb->surf[ i ].side = surff->getInt( surfid[ i ], SURF_SIDE );
			odb->surf[ i ].glow = surff->getInt( surfid[ i ], SURF_GLOW );
			odb->surf[ i ].line = surff->getInt( surfid[ i ], SURF_LINE );
			
			odb->surf[ i ].rimg = surff->getImg( surfid[ i ], SURF_RIMG );
			odb->surf[ i ].timg = surff->getImg( surfid[ i ], SURF_TIMG );
		}
		
		/* find surface index for each polygon */
		
		for ( i = 0; i < odb->npolygons; i++ ) {
			tag = mesh->polTag( mesh, odb->pol[ i ].id, LWPTAG_SURF );
			odb->pol[ i ].sindex = findSurf( odb, tag );
		}
		
		return 1;
}

#pragma warning (default:4995)
