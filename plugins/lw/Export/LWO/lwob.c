/*
======================================================================
lwob.c

Functions for an LWOB reader.  LWOB is the LightWave object format
for versions of LW prior to 6.0.

Ernie Wright  17 Sep 00
====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lwo2.h"


/* IDs specific to LWOB */

#define ID_SRFS  LWID_('S','R','F','S')
#define ID_FLAG  LWID_('F','L','A','G')
#define ID_VLUM  LWID_('V','L','U','M')
#define ID_VDIF  LWID_('V','D','I','F')
#define ID_VSPC  LWID_('V','S','P','C')
#define ID_RFLT  LWID_('R','F','L','T')
#define ID_BTEX  LWID_('B','T','E','X')
#define ID_CTEX  LWID_('C','T','E','X')
#define ID_DTEX  LWID_('D','T','E','X')
#define ID_LTEX  LWID_('L','T','E','X')
#define ID_RTEX  LWID_('R','T','E','X')
#define ID_STEX  LWID_('S','T','E','X')
#define ID_TTEX  LWID_('T','T','E','X')
#define ID_TFLG  LWID_('T','F','L','G')
#define ID_TSIZ  LWID_('T','S','I','Z')
#define ID_TCTR  LWID_('T','C','T','R')
#define ID_TFAL  LWID_('T','F','A','L')
#define ID_TVEL  LWID_('T','V','E','L')
#define ID_TCLR  LWID_('T','C','L','R')
#define ID_TVAL  LWID_('T','V','A','L')
#define ID_TAMP  LWID_('T','A','M','P')
#define ID_TIMG  LWID_('T','I','M','G')
#define ID_TAAS  LWID_('T','A','A','S')
#define ID_TREF  LWID_('T','R','E','F')
#define ID_SDAT  LWID_('S','D','A','T')


/*
======================================================================
add_clip()

Add a clip to the clip list.  Used to store the contents of an RIMG or
TIMG surface subchunk.
====================================================================== */

static int add_clip( char *s, lwClip **clist, int *nclips )
{
   lwClip *clip;
   char *p;

   clip = calloc( 1, sizeof( lwClip ));
   if ( !clip ) return 0;

   clip->contrast.val = 1.0f;
   clip->brightness.val = 1.0f;
   clip->saturation.val = 1.0f;
   clip->gamma.val = 1.0f;

   if ( p = strstr( s, "(sequence)" )) {
      p[ -1 ] = 0;
      clip->type = ID_ISEQ;
      clip->source.seq.prefix = s;
      clip->source.seq.digits = 3;
   }
   else {
      clip->type = ID_STIL;
      clip->source.still.name = s;
   }

   *nclips++;
   clip->index = *nclips;

   lwListAdd( clist, clip );

   return clip->index;
}


/*
======================================================================
add_tvel()

Add a triple of envelopes to simulate the old texture velocity
parameters.
====================================================================== */

static int add_tvel( float pos[], float vel[], lwEnvelope **elist, int *nenvs )
{
   lwEnvelope *env;
   lwKey *key0, *key1;
   int i;

   for ( i = 0; i < 3; i++ ) {
      env = calloc( 1, sizeof( lwEnvelope ));
      key0 = calloc( 1, sizeof( lwKey ));
      key1 = calloc( 1, sizeof( lwKey ));
      if ( !env || !key0 || !key1 ) return 0;

      key0->next = key1;
      key0->value = pos[ i ];
      key0->time = 0.0f;
      key1->prev = key0;
      key1->value = pos[ i ] + vel[ i ] * 30.0f;
      key1->time = 1.0f;
      key0->shape = key1->shape = ID_LINE;

      env->index = *nenvs + i + 1;
      env->type = 0x0301 + i;
      env->name = malloc( 11 );
      if ( env->name ) {
         strcpy( env->name, "Position.X" );
         env->name[ 9 ] += i;
      }
      env->key = key0;
      env->nkeys = 2;
      env->behavior[ 0 ] = BEH_LINEAR;
      env->behavior[ 1 ] = BEH_LINEAR;

      lwListAdd( elist, env );
   }

   *nenvs += 3;
   return env->index - 2;
}


/*
======================================================================
get_texture()

Create a new texture for BTEX, CTEX, etc. subchunks.
====================================================================== */

static lwTexture *get_texture( char *s )
{
   lwTexture *tex;

   tex = calloc( 1, sizeof( lwTexture ));
   if ( !tex ) return NULL;

   tex->tmap.size.val[ 0 ] =
   tex->tmap.size.val[ 1 ] =
   tex->tmap.size.val[ 2 ] = 1.0f;
   tex->opacity.val = 1.0f;
   tex->enabled = 1;

   if ( strstr( s, "Image Map" )) {
      tex->type = ID_IMAP;
      if ( strstr( s, "Planar" ))           tex->param.imap.projection = 0;
      else if ( strstr( s, "Cylindrical" )) tex->param.imap.projection = 1;
      else if ( strstr( s, "Spherical" ))   tex->param.imap.projection = 2;
      else if ( strstr( s, "Cubic" ))       tex->param.imap.projection = 3;
      else if ( strstr( s, "Front" ))       tex->param.imap.projection = 4;
      tex->param.imap.aa_strength = 1.0f;
      tex->param.imap.amplitude.val = 1.0f;
      free( s );
   }
   else {
      tex->type = ID_PROC;
      tex->param.proc.name = s;
   }

   return tex;
}


/*
======================================================================
lwGetSurface5()

Read an lwSurface from an LWOB file.
====================================================================== */

lwSurface *lwGetSurface5( FILE *fp, int cksize, lwObject *obj )
{
   lwSurface *surf;
   lwTexture *tex;
   lwPlugin *shdr;
   char *s;
   float v[ 3 ];
   unsigned int id, flags;
   unsigned short sz;
   int pos, rlen, i;


   /* allocate the Surface structure */

   surf = calloc( 1, sizeof( lwSurface ));
   if ( !surf ) goto Fail;

   /* non-zero defaults */

   surf->color.rgb[ 0 ] = 0.78431f;
   surf->color.rgb[ 1 ] = 0.78431f;
   surf->color.rgb[ 2 ] = 0.78431f;
   surf->diffuse.val    = 1.0f;
   surf->glossiness.val = 0.4f;
   surf->bump.val       = 1.0f;
   surf->eta.val        = 1.0f;
   surf->sideflags      = 1;

   /* remember where we started */

   set_flen( 0 );
   pos = ftell( fp );

   /* name */

   surf->name = getS0( fp );

   /* first subchunk header */

   id = getU4( fp );
   sz = getU2( fp );
   if ( 0 > get_flen() ) goto Fail;

   /* process subchunks as they're encountered */

   while ( 1 ) {
      sz += sz & 1;
      set_flen( 0 );

      switch ( id ) {
         case ID_COLR:
            surf->color.rgb[ 0 ] = getU1( fp ) / 255.0f;
            surf->color.rgb[ 1 ] = getU1( fp ) / 255.0f;
            surf->color.rgb[ 2 ] = getU1( fp ) / 255.0f;
            break;

         case ID_FLAG:
            flags = getU2( fp );
            if ( flags &   4 ) surf->smooth = 1.56207f;
            if ( flags &   8 ) surf->color_hilite.val = 1.0f;
            if ( flags &  16 ) surf->color_filter.val = 1.0f;
            if ( flags & 128 ) surf->dif_sharp.val = 0.5f;
            if ( flags & 256 ) surf->sideflags = 3;
            if ( flags & 512 ) surf->add_trans.val = 1.0f;
            break;

         case ID_LUMI:
            surf->luminosity.val = getI2( fp ) / 256.0f;
            break;

         case ID_VLUM:
            surf->luminosity.val = getF4( fp );
            break;

         case ID_DIFF:
            surf->diffuse.val = getI2( fp ) / 256.0f;
            break;

         case ID_VDIF:
            surf->diffuse.val = getF4( fp );
            break;

         case ID_SPEC:
            surf->specularity.val = getI2( fp ) / 256.0f;
            break;

         case ID_VSPC:
            surf->specularity.val = getF4( fp );
            break;

         case ID_GLOS:
            surf->glossiness.val = ( float ) log( getU2( fp )) / 20.7944f;
            break;

         case ID_SMAN:
            surf->smooth = getF4( fp );
            break;

         case ID_REFL:
            surf->reflection.val.val = getI2( fp ) / 256.0f;
            break;

         case ID_RFLT:
            surf->reflection.options = getU2( fp );
            break;

         case ID_RIMG:
            s = getS0( fp );
            surf->reflection.cindex = add_clip( s, &obj->clip, &obj->nclips );
            surf->reflection.options = 3;
            break;

         case ID_RSAN:
            surf->reflection.seam_angle = getF4( fp );
            break;

         case ID_TRAN:
            surf->transparency.val.val = getI2( fp ) / 256.0f;
            break;

         case ID_RIND:
            surf->eta.val = getF4( fp );
            break;

         case ID_BTEX:
            s = getbytes( fp, sz );
            tex = get_texture( s );
            lwListAdd( &surf->bump.tex, tex );
            break;

         case ID_CTEX:
            s = getbytes( fp, sz );
            tex = get_texture( s );
            lwListAdd( &surf->color.tex, tex );
            break;

         case ID_DTEX:
            s = getbytes( fp, sz );
            tex = get_texture( s );
            lwListAdd( &surf->diffuse.tex, tex );
            break;

         case ID_LTEX:
            s = getbytes( fp, sz );
            tex = get_texture( s );
            lwListAdd( &surf->luminosity.tex, tex );
            break;

         case ID_RTEX:
            s = getbytes( fp, sz );
            tex = get_texture( s );
            lwListAdd( &surf->reflection.val.tex, tex );
            break;

         case ID_STEX:
            s = getbytes( fp, sz );
            tex = get_texture( s );
            lwListAdd( &surf->specularity.tex, tex );
            break;

         case ID_TTEX:
            s = getbytes( fp, sz );
            tex = get_texture( s );
            lwListAdd( &surf->transparency.val.tex, tex );
            break;

         case ID_TFLG:
            flags = getU2( fp );

            if ( flags & 1 ) i = 0;
            if ( flags & 2 ) i = 1;
            if ( flags & 4 ) i = 2;
            tex->axis = i;
            if ( tex->type == ID_IMAP )
               tex->param.imap.axis = i;
            else
               tex->param.proc.axis = i;

            if ( flags &  8 ) tex->tmap.coord_sys = 1;
            if ( flags & 16 ) tex->negative = 1;
            if ( flags & 32 ) tex->param.imap.pblend = 1;
            if ( flags & 64 ) {
               tex->param.imap.aa_strength = 1.0f;
               tex->param.imap.aas_flags = 1;
            }
            break;

         case ID_TSIZ:
            for ( i = 0; i < 3; i++ )
               tex->tmap.size.val[ i ] = getF4( fp );
            break;

         case ID_TCTR:
            for ( i = 0; i < 3; i++ )
               tex->tmap.center.val[ i ] = getF4( fp );
            break;

         case ID_TFAL:
            for ( i = 0; i < 3; i++ )
               tex->tmap.falloff.val[ i ] = getF4( fp );
            break;

         case ID_TVEL:
            for ( i = 0; i < 3; i++ )
               v[ i ] = getF4( fp );
            tex->tmap.center.eindex = add_tvel( tex->tmap.center.val, v,
               &obj->env, &obj->nenvs );
            break;

         case ID_TCLR:
            for ( i = 0; i < 3; i++ )
               tex->param.proc.value[ i ] = getU1( fp ) / 255.0f;
            break;

         case ID_TVAL:
            tex->param.proc.value[ 0 ] = getI2( fp ) / 256.0f;
            break;

         case ID_TAMP:
            if ( tex->type == ID_IMAP )
               tex->param.imap.amplitude.val = getF4( fp );
            break;

         case ID_TIMG:
            s = getS0( fp );
            tex->param.imap.cindex = add_clip( s, &obj->clip, &obj->nclips );
            break;

         case ID_TAAS:
            tex->param.imap.aa_strength = getF4( fp );
            tex->param.imap.aas_flags = 1;
            break;

         case ID_TREF:
            tex->tmap.ref_object = getbytes( fp, sz );
            break;

         case ID_SHDR:
            shdr = calloc( 1, sizeof( lwPlugin ));
            if ( !shdr ) goto Fail;
            shdr->name = getbytes( fp, sz );
            lwListAdd( &surf->shader, shdr );
            surf->nshaders++;
            break;

         case ID_SDAT:
            shdr->data = getbytes( fp, sz );
            break;

         default:
            break;
      }

      /* error while reading current subchunk? */

      rlen = get_flen();
      if ( rlen < 0 || rlen > sz ) goto Fail;

      /* skip unread parts of the current subchunk */

      if ( rlen < sz )
         fseek( fp, sz - rlen, SEEK_CUR );

      /* end of the SURF chunk? */

      if ( cksize <= ftell( fp ) - pos )
         break;

      /* get the next subchunk header */

      set_flen( 0 );
      id = getU4( fp );
      sz = getU2( fp );
      if ( 6 != get_flen() ) goto Fail;
   }

   return surf;

Fail:
   if ( surf ) lwFreeSurface( surf );
   return NULL;
}


/*
======================================================================
lwGetPolygons5()

Read polygon records from a POLS chunk in an LWOB file.  The polygons
are added to the array in the lwPolygonList.
====================================================================== */

int lwGetPolygons5( FILE *fp, int cksize, lwPolygonList *plist, int ptoffset )
{
   lwPolygon *pp;
   lwPolVert *pv;
   unsigned char *buf, *bp;
   int i, j, nv, nverts, npols;


   if ( cksize == 0 ) return 1;

   /* read the whole chunk */

   set_flen( 0 );
   buf = getbytes( fp, cksize );
   if ( !buf ) goto Fail;

   /* count the polygons and vertices */

   nverts = 0;
   npols = 0;
   bp = buf;

   while ( bp < buf + cksize ) {
      nv = sgetU2( &bp );
      nverts += nv;
      npols++;
      bp += 2 * nv;
      i = sgetI2( &bp );
      if ( i < 0 ) bp += 2;      /* detail polygons */
   }

   if ( !lwAllocPolygons( plist, npols, nverts ))
      goto Fail;

   /* fill in the new polygons */

   bp = buf;
   pp = plist->pol + plist->offset;
   pv = plist->pol[ 0 ].v + plist->voffset;

   for ( i = 0; i < npols; i++ ) {
      nv = sgetU2( &bp );

      pp->nverts = nv;
      pp->type = ID_FACE;
      if ( !pp->v ) pp->v = pv;
      for ( j = 0; j < nv; j++ )
         pv[ j ].index = sgetU2( &bp ) + ptoffset;
      j = sgetI2( &bp );
      if ( j < 0 ) {
         j = -j;
         bp += 2;
      }
      j -= 1;
      pp->surf = ( lwSurface * ) j;

      pp++;
      pv += nv;
   }

   free( buf );
   return 1;

Fail:
   if ( buf ) free( buf );
   lwFreePolygons( plist );
   return 0;
}


/*
======================================================================
getLWObject5()

Returns the contents of an LWOB, given its filename, or NULL if the
file couldn't be loaded.  On failure, failID and failpos can be used
to diagnose the cause.

1.  If the file isn't an LWOB, failpos will contain 12 and failID will
    be unchanged.

2.  If an error occurs while reading an LWOB, failID will contain the
    most recently read IFF chunk ID, and failpos will contain the
    value returned by ftell() at the time of the failure.

3.  If the file couldn't be opened, or an error occurs while reading
    the first 12 bytes, both failID and failpos will be unchanged.

If you don't need this information, failID and failpos can be NULL.
====================================================================== */

lwObject *lwGetObject5( char *filename, unsigned int *failID, int *failpos )
{
   FILE *fp = NULL;
   lwObject *object;
   lwLayer *layer;
   lwNode *node;
   unsigned int id, formsize, type, cksize;


   /* open the file */

   fp = fopen( filename, "rb" );
   if ( !fp ) return NULL;

   /* read the first 12 bytes */

   set_flen( 0 );
   id       = getU4( fp );
   formsize = getU4( fp );
   type     = getU4( fp );
   if ( 12 != get_flen() ) {
      fclose( fp );
      return NULL;
   }

   /* LWOB? */

   if ( id != ID_FORM || type != ID_LWOB ) {
      fclose( fp );
      if ( failpos ) *failpos = 12;
      return NULL;
   }

   /* allocate an object and a default layer */

   object = calloc( 1, sizeof( lwObject ));
   if ( !object ) goto Fail;

   layer = calloc( 1, sizeof( lwLayer ));
   if ( !layer ) goto Fail;
   object->layer = layer;
   object->nlayers = 1;

   /* get the first chunk header */

   id = getU4( fp );
   cksize = getU4( fp );
   if ( 0 > get_flen() ) goto Fail;

   /* process chunks as they're encountered */

   while ( 1 ) {
      cksize += cksize & 1;

      switch ( id )
      {
         case ID_PNTS:
            if ( !lwGetPoints( fp, cksize, &layer->point ))
               goto Fail;
            break;

         case ID_POLS:
            if ( !lwGetPolygons5( fp, cksize, &layer->polygon,
               layer->point.offset ))
               goto Fail;
            break;

         case ID_SRFS:
            if ( !lwGetTags( fp, cksize, &object->taglist ))
               goto Fail;
            break;

         case ID_SURF:
            node = ( lwNode * ) lwGetSurface5( fp, cksize, object );
            if ( !node ) goto Fail;
            lwListAdd( &object->surf, node );
            object->nsurfs++;
            break;

         default:
            fseek( fp, cksize, SEEK_CUR );
            break;
      }

      /* end of the file? */

      if ( formsize <= ftell( fp ) - 8 ) break;

      /* get the next chunk header */

      set_flen( 0 );
      id = getU4( fp );
      cksize = getU4( fp );
      if ( 8 != get_flen() ) goto Fail;
   }

   fclose( fp );
   fp = NULL;

   lwGetBoundingBox( &layer->point, layer->bbox );
   lwGetPolyNormals( &layer->point, &layer->polygon );
   if ( !lwGetPointPolygons( &layer->point, &layer->polygon )) goto Fail;
   if ( !lwResolvePolySurfaces( &layer->polygon, &object->taglist,
      &object->surf, &object->nsurfs )) goto Fail;
   lwGetVertNormals( &layer->point, &layer->polygon );

   return object;

Fail:
   if ( failID ) *failID = id;
   if ( fp ) {
      if ( failpos ) *failpos = ftell( fp );
      fclose( fp );
   }
   lwFreeObject( object );
   return NULL;
}
