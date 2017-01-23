#include "stdafx.h"
/*
======================================================================
vecmat.c

Basic vector and matrix functions.
====================================================================== */
#include "vecmat.h"

#pragma warning (disable:4995)

float dot( LWFVector a, LWFVector b )
{
   return a[ 0 ] * b[ 0 ] + a[ 1 ] * b[ 1 ] + a[ 2 ] * b[ 2 ];
}


void cross( LWFVector a, LWFVector b, LWFVector c )
{
   c[ 0 ] = a[ 1 ] * b[ 2 ] - a[ 2 ] * b[ 1 ];
   c[ 1 ] = a[ 2 ] * b[ 0 ] - a[ 0 ] * b[ 2 ];
   c[ 2 ] = a[ 0 ] * b[ 1 ] - a[ 1 ] * b[ 0 ];
}


void normalize( LWFVector v )
{
   float r;

   r = ( float ) sqrt( dot( v, v ));
   if ( r > 0 ) {
      v[ 0 ] /= r;
      v[ 1 ] /= r;
      v[ 2 ] /= r;
   }
}

#pragma warning (default:4995)
