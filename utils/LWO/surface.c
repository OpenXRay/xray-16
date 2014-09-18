/*
======================================================================
surface.c

Surface functions for an LWO2 reader.

Ernie Wright  17 Sep 00
====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwo2.h"


/*
======================================================================
lwFreePlugin()

Free the memory used by an lwPlugin.
====================================================================== */

void lwFreePlugin( lwPlugin *p )
{
   if ( p ) {
      if ( p->ord ) free( p->ord );
      if ( p->name ) free( p->name );
      if ( p->data ) free( p->data );
      free( p );
   }
}


/*
======================================================================
lwFreeTexture()

Free the memory used by an lwTexture.
====================================================================== */

void lwFreeTexture( lwTexture *t )
{
   if ( t ) {
      if ( t->ord ) free( t->ord );
      switch ( t->type ) {
         case ID_IMAP:
            if ( t->param.imap.vmap_name ) free( t->param.imap.vmap_name );
            break;
         case ID_PROC:
            if ( t->param.proc.name ) free( t->param.proc.name );
            if ( t->param.proc.data ) free( t->param.proc.data );
            break;
         case ID_GRAD:
            if ( t->param.grad.key ) free( t->param.grad.key );
            if ( t->param.grad.ikey ) free( t->param.grad.ikey );
            break;
      }
      free( t );
   }
}


/*
======================================================================
lwFreeSurface()

Free the memory used by an lwSurface.
====================================================================== */

void lwFreeSurface( lwSurface *surf )
{
   if ( surf ) {
      if ( surf->name ) free( surf->name );
      if ( surf->srcname ) free( surf->srcname );

      lwListFree( surf->shader, lwFreePlugin );

      lwListFree( surf->color.tex, lwFreeTexture );
      lwListFree( surf->luminosity.tex, lwFreeTexture );
      lwListFree( surf->diffuse.tex, lwFreeTexture );
      lwListFree( surf->specularity.tex, lwFreeTexture );
      lwListFree( surf->glossiness.tex, lwFreeTexture );
      lwListFree( surf->reflection.val.tex, lwFreeTexture );
      lwListFree( surf->transparency.val.tex, lwFreeTexture );
      lwListFree( surf->eta.tex, lwFreeTexture );
      lwListFree( surf->translucency.tex, lwFreeTexture );
      lwListFree( surf->bump.tex, lwFreeTexture );

      free( surf );
   }
}


/*
======================================================================
lwGetTHeader()

Read a texture map header from a SURF.BLOK in an LWO2 file.  This is
the first subchunk in a BLOK, and its contents are common to all three
texture types.
====================================================================== */

int lwGetTHeader( FILE *fp, int hsz, lwTexture *tex )
{
   unsigned int id;
   unsigned short sz;
   int pos, rlen;


   /* remember where we started */

   set_flen( 0 );
   pos = ftell( fp );

   /* ordinal string */

   tex->ord = getS0( fp );

   /* first subchunk header */

   id = getU4( fp );
   sz = getU2( fp );
   if ( 0 > get_flen() ) return 0;

   /* process subchunks as they're encountered */

   while ( 1 ) {
      sz += sz & 1;
      set_flen( 0 );

      switch ( id ) {
         case ID_CHAN:
            tex->chan = getU4( fp );
            break;

         case ID_OPAC:
            tex->opac_type = getU2( fp );
            tex->opacity.val = getF4( fp );
            tex->opacity.eindex = getVX( fp );
            break;

         case ID_ENAB:
            tex->enabled = getU2( fp );
            break;

         case ID_NEGA:
            tex->negative = getU2( fp );
            break;

         case ID_AXIS:
            tex->axis = getU2( fp );
            break;

         default:
            break;
      }

      /* error while reading current subchunk? */

      rlen = get_flen();
      if ( rlen < 0 || rlen > sz ) return 0;

      /* skip unread parts of the current subchunk */

      if ( rlen < sz )
         fseek( fp, sz - rlen, SEEK_CUR );

      /* end of the texture header subchunk? */

      if ( hsz <= ftell( fp ) - pos )
         break;

      /* get the next subchunk header */

      set_flen( 0 );
      id = getU4( fp );
      sz = getU2( fp );
      if ( 6 != get_flen() ) return 0;
   }

   set_flen( ftell( fp ) - pos );
   return 1;
}


/*
======================================================================
lwGetTMap()

Read a texture map from a SURF.BLOK in an LWO2 file.  The TMAP
defines the mapping from texture to world or object coordinates.
====================================================================== */

int lwGetTMap( FILE *fp, int tmapsz, lwTMap *tmap )
{
   unsigned int id;
   unsigned short sz;
   int rlen, pos, i;

   pos = ftell( fp );
   id = getU4( fp );
   sz = getU2( fp );
   if ( 0 > get_flen() ) return 0;

   while ( 1 ) {
      sz += sz & 1;
      set_flen( 0 );

      switch ( id ) {
         case ID_SIZE:
            for ( i = 0; i < 3; i++ )
               tmap->size.val[ i ] = getF4( fp );
            tmap->size.eindex = getVX( fp );
            break;

         case ID_CNTR:
            for ( i = 0; i < 3; i++ )
               tmap->center.val[ i ] = getF4( fp );
            tmap->center.eindex = getVX( fp );
            break;

         case ID_ROTA:
            for ( i = 0; i < 3; i++ )
               tmap->rotate.val[ i ] = getF4( fp );
            tmap->rotate.eindex = getVX( fp );
            break;

         case ID_FALL:
            tmap->fall_type = getU2( fp );
            for ( i = 0; i < 3; i++ )
               tmap->falloff.val[ i ] = getF4( fp );
            tmap->falloff.eindex = getVX( fp );
            break;

         case ID_OREF:
            tmap->ref_object = getS0( fp );
            break;

         case ID_CSYS:
            tmap->coord_sys = getU2( fp );
            break;

         default:
            break;
      }

      /* error while reading the current subchunk? */

      rlen = get_flen();
      if ( rlen < 0 || rlen > sz ) return 0;

      /* skip unread parts of the current subchunk */

      if ( rlen < sz )
         fseek( fp, sz - rlen, SEEK_CUR );

      /* end of the TMAP subchunk? */

      if ( tmapsz <= ftell( fp ) - pos )
         break;

      /* get the next subchunk header */

      set_flen( 0 );
      id = getU4( fp );
      sz = getU2( fp );
      if ( 6 != get_flen() ) return 0;
   }

   set_flen( ftell( fp ) - pos );
   return 1;
}


/*
======================================================================
lwGetImageMap()

Read an lwImageMap from a SURF.BLOK in an LWO2 file.
====================================================================== */

int lwGetImageMap( FILE *fp, int rsz, lwTexture *tex )
{
   unsigned int id;
   unsigned short sz;
   int rlen, pos;

   pos = ftell( fp );
   id = getU4( fp );
   sz = getU2( fp );
   if ( 0 > get_flen() ) return 0;

   while ( 1 ) {
      sz += sz & 1;
      set_flen( 0 );

      switch ( id ) {
         case ID_TMAP:
            if ( !lwGetTMap( fp, sz, &tex->tmap )) return 0;
            break;

         case ID_PROJ:
            tex->param.imap.projection = getU2( fp );
            break;

         case ID_VMAP:
            tex->param.imap.vmap_name = getS0( fp );
            break;

         case ID_AXIS:
            tex->param.imap.axis = getU2( fp );
            break;

         case ID_IMAG:
            tex->param.imap.cindex = getVX( fp );
            break;

         case ID_WRAP:
            tex->param.imap.wrapw_type = getU2( fp );
            tex->param.imap.wraph_type = getU2( fp );
            break;

         case ID_WRPW:
            tex->param.imap.wrapw.val = getF4( fp );
            tex->param.imap.wrapw.eindex = getVX( fp );
            break;

         case ID_WRPH:
            tex->param.imap.wraph.val = getF4( fp );
            tex->param.imap.wraph.eindex = getVX( fp );
            break;

         case ID_AAST:
            tex->param.imap.aas_flags = getU2( fp );
            tex->param.imap.aa_strength = getF4( fp );
            break;

         case ID_PIXB:
            tex->param.imap.pblend = getU2( fp );
            break;

         case ID_STCK:
            tex->param.imap.stck.val = getF4( fp );
            tex->param.imap.stck.eindex = getVX( fp );
            break;

         case ID_TAMP:
            tex->param.imap.amplitude.val = getF4( fp );
            tex->param.imap.amplitude.eindex = getVX( fp );
            break;

         default:
            break;
      }

      /* error while reading the current subchunk? */

      rlen = get_flen();
      if ( rlen < 0 || rlen > sz ) return 0;

      /* skip unread parts of the current subchunk */

      if ( rlen < sz )
         fseek( fp, sz - rlen, SEEK_CUR );

      /* end of the image map? */

      if ( rsz <= ftell( fp ) - pos )
         break;

      /* get the next subchunk header */

      set_flen( 0 );
      id = getU4( fp );
      sz = getU2( fp );
      if ( 6 != get_flen() ) return 0;
   }

   set_flen( ftell( fp ) - pos );
   return 1;
}


/*
======================================================================
lwGetProcedural()

Read an lwProcedural from a SURF.BLOK in an LWO2 file.
====================================================================== */

int lwGetProcedural( FILE *fp, int rsz, lwTexture *tex )
{
   unsigned int id;
   unsigned short sz;
   int rlen, pos;

   pos = ftell( fp );
   id = getU4( fp );
   sz = getU2( fp );
   if ( 0 > get_flen() ) return 0;

   while ( 1 ) {
      sz += sz & 1;
      set_flen( 0 );

      switch ( id ) {
         case ID_TMAP:
            if ( !lwGetTMap( fp, sz, &tex->tmap )) return 0;
            break;

         case ID_AXIS:
            tex->param.proc.axis = getU2( fp );
            break;

         case ID_VALU:
            tex->param.proc.value[ 0 ] = getF4( fp );
            if ( sz >= 8 ) tex->param.proc.value[ 1 ] = getF4( fp );
            if ( sz >= 12 ) tex->param.proc.value[ 2 ] = getF4( fp );
            break;

         case ID_FUNC:
            tex->param.proc.name = getS0( fp );
            rlen = get_flen();
            tex->param.proc.data = getbytes( fp, sz - rlen );
            break;

         default:
            break;
      }

      /* error while reading the current subchunk? */

      rlen = get_flen();
      if ( rlen < 0 || rlen > sz ) return 0;

      /* skip unread parts of the current subchunk */

      if ( rlen < sz )
         fseek( fp, sz - rlen, SEEK_CUR );

      /* end of the procedural block? */

      if ( rsz <= ftell( fp ) - pos )
         break;

      /* get the next subchunk header */

      set_flen( 0 );
      id = getU4( fp );
      sz = getU2( fp );
      if ( 6 != get_flen() ) return 0;
   }

   set_flen( ftell( fp ) - pos );
   return 1;
}


/*
======================================================================
lwGetGradient()

Read an lwGradient from a SURF.BLOK in an LWO2 file.
====================================================================== */

int lwGetGradient( FILE *fp, int rsz, lwTexture *tex )
{
   unsigned int id;
   unsigned short sz;
   int rlen, pos, i, j, nkeys;

   pos = ftell( fp );
   id = getU4( fp );
   sz = getU2( fp );
   if ( 0 > get_flen() ) return 0;

   while ( 1 ) {
      sz += sz & 1;
      set_flen( 0 );

      switch ( id ) {
         case ID_TMAP:
            if ( !lwGetTMap( fp, sz, &tex->tmap )) return 0;
            break;

         case ID_PNAM:
            tex->param.grad.paramname = getS0( fp );
            break;

         case ID_INAM:
            tex->param.grad.itemname = getS0( fp );
            break;

         case ID_GRST:
            tex->param.grad.start = getF4( fp );
            break;

         case ID_GREN:
            tex->param.grad.end = getF4( fp );
            break;

         case ID_GRPT:
            tex->param.grad.repeat = getU2( fp );
            break;

         case ID_FKEY:
            nkeys = sz / sizeof( lwGradKey );
            tex->param.grad.key = calloc( nkeys, sizeof( lwGradKey ));
            if ( !tex->param.grad.key ) return 0;
            for ( i = 0; i < nkeys; i++ ) {
               tex->param.grad.key[ i ].value = getF4( fp );
               for ( j = 0; j < 4; j++ )
                  tex->param.grad.key[ i ].rgba[ j ] = getF4( fp );
            }
            break;

         case ID_IKEY:
            nkeys = sz / 2;
            tex->param.grad.ikey = calloc( nkeys, sizeof( short ));
            if ( !tex->param.grad.ikey ) return 0;
            for ( i = 0; i < nkeys; i++ )
               tex->param.grad.ikey[ i ] = getU2( fp );
            break;

         default:
            break;
      }

      /* error while reading the current subchunk? */

      rlen = get_flen();
      if ( rlen < 0 || rlen > sz ) return 0;

      /* skip unread parts of the current subchunk */

      if ( rlen < sz )
         fseek( fp, sz - rlen, SEEK_CUR );

      /* end of the gradient? */

      if ( rsz <= ftell( fp ) - pos )
         break;

      /* get the next subchunk header */

      set_flen( 0 );
      id = getU4( fp );
      sz = getU2( fp );
      if ( 6 != get_flen() ) return 0;
   }

   set_flen( ftell( fp ) - pos );
   return 1;
}


/*
======================================================================
lwGetTexture()

Read an lwTexture from a SURF.BLOK in an LWO2 file.
====================================================================== */

lwTexture *lwGetTexture( FILE *fp, int bloksz, unsigned int type )
{
   lwTexture *tex;
   unsigned short sz;
   int ok;

   tex = calloc( 1, sizeof( lwTexture ));
   if ( !tex ) return NULL;

   tex->type = type;
   tex->tmap.size.val[ 0 ] =
   tex->tmap.size.val[ 1 ] =
   tex->tmap.size.val[ 2 ] = 1.0f;
   tex->opacity.val = 1.0f;
   tex->enabled = 1;

   sz = getU2( fp );
   if ( !lwGetTHeader( fp, sz, tex )) {
      free( tex );
      return NULL;
   }

   sz = bloksz - sz - 6;
   switch ( type ) {
      case ID_IMAP:  ok = lwGetImageMap( fp, sz, tex );  break;
      case ID_PROC:  ok = lwGetProcedural( fp, sz, tex );  break;
      case ID_GRAD:  ok = lwGetGradient( fp, sz, tex );  break;
      default:
         ok = !fseek( fp, sz, SEEK_CUR );
   }

   if ( !ok ) {
      lwFreeTexture( tex );
      return NULL;
   }

   set_flen( bloksz );
   return tex;
}


/*
======================================================================
lwGetShader()

Read a shader record from a SURF.BLOK in an LWO2 file.
====================================================================== */

lwPlugin *lwGetShader( FILE *fp, int bloksz )
{
   lwPlugin *shdr;
   unsigned int id;
   unsigned short sz;
   int hsz, rlen, pos;

   shdr = calloc( 1, sizeof( lwPlugin ));
   if ( !shdr ) return NULL;

   pos = ftell( fp );
   set_flen( 0 );
   hsz = getU2( fp );
   shdr->ord = getS0( fp );
   id = getU4( fp );
   sz = getU2( fp );
   if ( 0 > get_flen() ) goto Fail;

   while ( hsz > 0 ) {
      sz += sz & 1;
      hsz -= sz;
      if ( id == ID_ENAB ) {
         shdr->flags = getU2( fp );
         break;
      }
      else {
         fseek( fp, sz, SEEK_CUR );
         id = getU4( fp );
         sz = getU2( fp );
      }
   }

   id = getU4( fp );
   sz = getU2( fp );
   if ( 0 > get_flen() ) goto Fail;

   while ( 1 ) {
      sz += sz & 1;
      set_flen( 0 );

      switch ( id ) {
         case ID_FUNC:
            shdr->name = getS0( fp );
            rlen = get_flen();
            shdr->data = getbytes( fp, sz - rlen );
            break;

         default:
            break;
      }

      /* error while reading the current subchunk? */

      rlen = get_flen();
      if ( rlen < 0 || rlen > sz ) goto Fail;

      /* skip unread parts of the current subchunk */

      if ( rlen < sz )
         fseek( fp, sz - rlen, SEEK_CUR );

      /* end of the shader block? */

      if ( bloksz <= ftell( fp ) - pos )
         break;

      /* get the next subchunk header */

      set_flen( 0 );
      id = getU4( fp );
      sz = getU2( fp );
      if ( 6 != get_flen() ) goto Fail;
   }

   set_flen( ftell( fp ) - pos );
   return shdr;

Fail:
   lwFreePlugin( shdr );
   return NULL;
}


/*
======================================================================
compare_textures()
compare_shaders()

Callbacks for the lwListInsert() function, which is called to add
textures to surface channels and shaders to surfaces.
====================================================================== */

static int compare_textures( lwTexture *a, lwTexture *b )
{
   return strcmp( a->ord, b->ord );
}


static int compare_shaders( lwPlugin *a, lwPlugin *b )
{
   return strcmp( a->ord, b->ord );
}


/*
======================================================================
add_texture()

Finds the surface channel (lwTParam or lwCParam) to which a texture is
applied, then calls lwListInsert().
====================================================================== */

static int add_texture( lwSurface *surf, lwTexture *tex )
{
   lwTexture **list;

   switch ( tex->chan ) {
      case ID_COLR:  list = &surf->color.tex;             break;
      case ID_LUMI:  list = &surf->luminosity.tex;        break;
      case ID_DIFF:  list = &surf->diffuse.tex;           break;
      case ID_SPEC:  list = &surf->specularity.tex;       break;
      case ID_GLOS:  list = &surf->glossiness.tex;        break;
      case ID_REFL:  list = &surf->reflection.val.tex;    break;
      case ID_TRAN:  list = &surf->transparency.val.tex;  break;
      case ID_RIND:  list = &surf->eta.tex;               break;
      case ID_TRNL:  list = &surf->translucency.tex;      break;
      case ID_BUMP:  list = &surf->bump.tex;              break;
      default:  return 0;
   }

   lwListInsert( list, tex, compare_textures );
   return 1;
}


/*
======================================================================
lwDefaultSurface()

Allocate and initialize a surface.
====================================================================== */

lwSurface *lwDefaultSurface( void )
{
   lwSurface *surf;

   surf = calloc( 1, sizeof( lwSurface ));
   if ( !surf ) return NULL;

   surf->color.rgb[ 0 ] = 0.78431f;
   surf->color.rgb[ 1 ] = 0.78431f;
   surf->color.rgb[ 2 ] = 0.78431f;
   surf->diffuse.val    = 1.0f;
   surf->glossiness.val = 0.4f;
   surf->bump.val       = 1.0f;
   surf->eta.val        = 1.0f;
   surf->sideflags      = 1;

   return surf;
}


/*
======================================================================
lwGetSurface()

Read an lwSurface from an LWO2 file.
====================================================================== */

lwSurface *lwGetSurface( FILE *fp, int cksize )
{
   lwSurface *surf;
   lwTexture *tex;
   lwPlugin *shdr;
   unsigned int id, type;
   unsigned short sz;
   int pos, rlen;


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

   /* names */

   surf->name = getS0( fp );
   surf->srcname = getS0( fp );

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
            surf->color.rgb[ 0 ] = getF4( fp );
            surf->color.rgb[ 1 ] = getF4( fp );
            surf->color.rgb[ 2 ] = getF4( fp );
            surf->color.eindex = getVX( fp );
            break;

         case ID_LUMI:
            surf->luminosity.val = getF4( fp );
            surf->luminosity.eindex = getVX( fp );
            break;

         case ID_DIFF:
            surf->diffuse.val = getF4( fp );
            surf->diffuse.eindex = getVX( fp );
            break;

         case ID_SPEC:
            surf->specularity.val = getF4( fp );
            surf->specularity.eindex = getVX( fp );
            break;

         case ID_GLOS:
            surf->glossiness.val = getF4( fp );
            surf->glossiness.eindex = getVX( fp );
            break;

         case ID_REFL:
            surf->reflection.val.val = getF4( fp );
            surf->reflection.val.eindex = getVX( fp );
            break;

         case ID_RFOP:
            surf->reflection.options = getU2( fp );
            break;

         case ID_RIMG:
            surf->reflection.cindex = getVX( fp );
            break;

         case ID_RSAN:
            surf->reflection.seam_angle = getF4( fp );
            break;

         case ID_TRAN:
            surf->transparency.val.val = getF4( fp );
            surf->transparency.val.eindex = getVX( fp );
            break;

         case ID_TROP:
            surf->transparency.options = getU2( fp );
            break;

         case ID_TIMG:
            surf->transparency.cindex = getVX( fp );
            break;

         case ID_RIND:
            surf->eta.val = getF4( fp );
            surf->eta.eindex = getVX( fp );
            break;

         case ID_TRNL:
            surf->translucency.val = getF4( fp );
            surf->translucency.eindex = getVX( fp );
            break;

         case ID_BUMP:
            surf->bump.val = getF4( fp );
            surf->bump.eindex = getVX( fp );
            break;

         case ID_SMAN:
            surf->smooth = getF4( fp );
            break;

         case ID_SIDE:
            surf->sideflags = getU2( fp );
            break;

         case ID_CLRH:
            surf->color_hilite.val = getF4( fp );
            surf->color_hilite.eindex = getVX( fp );
            break;

         case ID_CLRF:
            surf->color_filter.val = getF4( fp );
            surf->color_filter.eindex = getVX( fp );
            break;

         case ID_ADTR:
            surf->add_trans.val = getF4( fp );
            surf->add_trans.eindex = getVX( fp );
            break;

         case ID_SHRP:
            surf->dif_sharp.val = getF4( fp );
            surf->dif_sharp.eindex = getVX( fp );
            break;

         case ID_GVAL:
            surf->glow.val = getF4( fp );
            surf->glow.eindex = getVX( fp );
            break;

         case ID_LINE:
            surf->line.enabled = 1;
            if ( sz >= 2 ) surf->line.flags = getU2( fp );
            if ( sz >= 6 ) surf->line.size.val = getF4( fp );
            if ( sz >= 8 ) surf->line.size.eindex = getVX( fp );
            break;

         case ID_ALPH:
            surf->alpha_mode = getU2( fp );
            surf->alpha = getF4( fp );
            break;

         case ID_AVAL:
            surf->alpha = getF4( fp );
            break;

         case ID_BLOK:
            type = getU4( fp );

            switch ( type ) {
               case ID_IMAP:
               case ID_PROC:
               case ID_GRAD:
                  tex = lwGetTexture( fp, sz - 4, type );
                  if ( !tex ) goto Fail;
                  if ( !add_texture( surf, tex ))
                     lwFreeTexture( tex );
                  set_flen( 4 + get_flen() );
                  break;
               case ID_SHDR:
                  shdr = lwGetShader( fp, sz - 4 );
                  if ( !shdr ) goto Fail;
                  lwListInsert( &surf->shader, shdr, compare_shaders );
                  ++surf->nshaders;
                  set_flen( 4 + get_flen() );
                  break;
            }
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
