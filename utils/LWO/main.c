/*
======================================================================
main.c

Test the LightWave object loader.

Ernie Wright  17 Sep 00

This is a command-line program that takes an object filename as its
argument, loads the file, and displays some statistics.  The filename
can include wildcards, and if it's "*.*", the program will also look
in subdirectories for objects to load.

We have to use platform-specific code, since C has no native method
for traversing a file system.  This version uses the MSVC runtime.
====================================================================== */

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lwo2.h"

#pragma warning (disable:4996)

int nobjects = 0, nlayers = 0, nsurfs = 0, nenvs = 0, nclips = 0,
   npoints = 0, npolygons = 0;


int testload( char *filename, unsigned int *failID, int *failpos )
{
   lwObject *obj;

   obj = lwGetObject( filename, failID, failpos );

   if ( obj ) {
      printf(
         "Layers:  %d\n"
         "Surfaces:  %d\n"
         "Envelopes:  %d\n"
         "Clips:  %d\n"
         "Points (first layer):  %d\n"
         "Polygons (first layer):  %d\n\n",
         obj->nlayers, obj->nsurfs, obj->nenvs, obj->nclips,
         obj->layer->point.count, obj->layer->polygon.count );
      nobjects++;
      nlayers += obj->nlayers;
      nsurfs += obj->nsurfs;
      nenvs += obj->nenvs;
      nclips += obj->nclips;
      npoints += obj->layer->point.count;
      npolygons += obj->layer->polygon.count;

      lwFreeObject( obj );
      return 1;
   }
   else {
      printf( "Couldn't load %s.\n", filename );
      return 0;
   }
}


int make_filename( char *spec, char *name, char *fullname )
{
   char
      drive[ _MAX_DRIVE ],
      dir[ _MAX_DIR ],
      node[ _MAX_FNAME ],
      ext[ _MAX_EXT ];

   _splitpath( spec, drive, dir, node, ext );
   _makepath( fullname, drive, dir, name, NULL );
   return 1;
}


int make_filespec( char *spec, char *subdir, char *fullname )
{
   char
      name[ _MAX_FNAME ],
      drive[ _MAX_DRIVE ],
      dir[ _MAX_DIR ],
      node[ _MAX_FNAME ],
      ext[ _MAX_EXT ];

   _splitpath( spec, drive, dir, node, ext );
   _makepath( name, drive, dir, subdir, NULL );
   _makepath( fullname, NULL, name, node, ext );
   return 1;
}


int find_files( char *filespec )
{
   long h, err;
   struct _finddata_t data;
   char *filename, *prevname;
   unsigned int failID;
   int failpos;

   filename = malloc( 520 );
   if ( !filename ) return 0;
   prevname = filename + 260;

   err = h = _findfirst( filespec, &data );
   if ( err == -1 ) {
      printf( "No files found: '%s'\n", filespec );
      return 0;
   }

   while ( err != -1 ) {
      if (( data.attrib & _A_SUBDIR ) && data.name[ 0 ] != '.' ) {
         make_filespec( filespec, data.name, filename );
         find_files( filename );
      }
      if ( !( data.attrib & _A_SUBDIR )) {
         make_filename( filespec, data.name, filename );
         if ( !strcmp( filename, prevname )) break;
         strcpy( prevname, filename );
         printf( "%s\n", filename );
         failID = failpos = 0;
         if ( !testload( filename, &failID, &failpos )) {
            printf( "%s\nLoading failed near byte %d\n\n", filename, failpos );
         }
      }
      err = _findnext( h, &data );
   }

   _findclose( h );
   free( filename );
   return 1;
}


void main( int argc, char *argv[] )
{
   float t1, t2;

   if ( argc != 2 ) {
      printf( "Usage:  %s <filespec>\n", argv[ 0 ] );
      exit( 0 );
   }

   t1 = ( float ) clock() / CLOCKS_PER_SEC;
   find_files( argv[ 1 ] );
   t2 = ( float ) clock() / CLOCKS_PER_SEC - t1;

   printf( "\n%8d objects\n", nobjects );
   printf( "%8d layers\n", nlayers );
   printf( "%8d surfaces\n", nsurfs );
   printf( "%8d envelopes\n", nenvs );
   printf( "%8d clips\n", nclips );
   printf( "%8d points\n", npoints );
   printf( "%8d polygons\n\n", npolygons );
   printf( "%g seconds\n\n", t2 );
}

#pragma warning (default:4996)
