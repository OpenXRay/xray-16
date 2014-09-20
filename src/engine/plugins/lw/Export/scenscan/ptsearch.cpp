#include "stdafx.h"
/*
======================================================================
ptsearch.c

Functions for searching an ObjectDB point array by coordinate value.

This is useful when you need to be able to quickly find a point in the
object database by matching a position.  An auxiliary array of point
indexes is created in initPointSearch() that sorts the point list by
one of the position coordinates.  We can then binary search on that
coordinate to find the point.

This is probably overkill for small objects, but the time required for
a simple linear search of the points grows as the square of the number
of points.
====================================================================== */
#include "objectdb.h"

#define KEY            (odb->vsortkey % 3)
#define IPOS           (odb->vsortkey / 3)
#define ARRAY( i )     odb->vsort[ i ]

#undef ARRAYSIZE
#define ARRAYSIZE      odb->npoints
#define COORD( p, c )  odb->pt[ odb->vsort[ p ]].pos[ ipos ][ c ]
#define SORTVAL( p )   COORD( p, key )

#pragma warning (disable:4995)



/*
======================================================================
vsortq()

Quicksort the point array.  This stops sorting when the partition size
falls below SUBARRAYSIZE, beyond which it's somewhat more efficient to
let a different kind of sort finish the job.
====================================================================== */

#define SUBARRAYSIZE  10
#define PUSH( i )     stack[ ++tos ] = i
#define POP( i )      i = stack[ tos-- ]
#define STACKEMPTY    tos < 0
#define SWAP( i, j )  t = i; i = j; j = t

static void vsortq( ObjectDB *odb )
{
   static int stack[ 200 ];
   int i, j, lt, rt, tos = -1, t, key = KEY, ipos = IPOS;
   float v;

   lt = 0;
   rt = ARRAYSIZE - 1;

   for ( ;; ) {
      while ( rt - lt > SUBARRAYSIZE ) {
         v = SORTVAL( rt );  i = lt - 1;  j = rt;
         for ( ;; ) {
            while ( SORTVAL( ++i ) < v ) ;
            while ( j && SORTVAL( --j ) > v ) ;
            if ( i > j ) break;
            SWAP( ARRAY( i ), ARRAY( j ));
         }
         SWAP( ARRAY( i ), ARRAY( rt ));
         if ( i - lt > rt - i )
            { PUSH( lt ); PUSH( i - 1 ); lt = i + 1; }
         else
            { PUSH( i + 1 ); PUSH( rt ); rt = i - 1; }
      }
      if ( STACKEMPTY ) break;
      POP( rt );
      POP( lt );
   }
}


/*
======================================================================
vsorti()

Insertion sort the point array.  Because the array was preconditioned
by a quicksort, it's almost sorted already, a best case for insertion.
====================================================================== */

static void vsorti( ObjectDB *odb )
{
   int i, j, t, key = KEY, ipos = IPOS;
   float v;

   for ( i = 1; i < ARRAYSIZE; i++ ) {
      t = ARRAY( i );
      v = SORTVAL( i );
      j = i;
      while ( j && SORTVAL( j - 1 ) > v ) {
         ARRAY( j ) = ARRAY( j - 1 );
         --j;
      }
      ARRAY( j ) = t;
   }
}


/*
======================================================================
initPointSearch()

Decides which of the three position coordinates to sort on, allocates
the auxiliary array of point indexes, and calls the sort routines.

We find the mean and deviation of each of the three axes and choose to
sort on the coordinate with the largest deviation.  This should reduce
the amount of hunting around required in the search due to duplicate
coordinate values.

The second argument is 0 to sort on initial positions and 1 to sort on
final positions.
====================================================================== */

int initPointSearch( ObjectDB *odb, int ipos )
{
   float m[ 3 ] = { 0.0f }, sd[ 3 ] = { 0.0f };
   int i, j;

   if ( odb->npoints <= 0 ) return 1;

   for ( i = 0; i < odb->npoints; i++ )
      for ( j = 0; j < 3; j++ )
         m[ j ] += odb->pt[ i ].pos[ ipos ][ j ];

   for ( j = 0; j < 3; j++ )
      m[ j ] /= odb->npoints;

   for ( i = 0; i < odb->npoints; i++ )
      for ( j = 0; j < 3; j++ )
         sd[ j ] += odb->pt[ i ].pos[ ipos ][ j ] - m[ j ];

   odb->vsortkey = sd[ 0 ] > sd[ 1 ] ?
      ( sd[ 0 ] > sd[ 2 ] ? 0 : 2 ) :
      ( sd[ 1 ] > sd[ 2 ] ? 1 : 2 );
   odb->vsortkey += 3 * ipos;

   odb->vsort = (int*)calloc( odb->npoints, sizeof( int ));
   if ( !odb->vsort ) return 0;

   for ( i = 0; i < odb->npoints; i++ )
      odb->vsort[ i ] = i;

   vsortq( odb );
   vsorti( odb );

   return 1;
}


/*
======================================================================
freePointSearch()

Free memory allocated by initPointSearch().
====================================================================== */

void freePointSearch( ObjectDB *odb )
{
   if ( odb ) {
      if ( odb->vsort ) {
         free( odb->vsort );
         odb->vsort = NULL;
      }
   }
}


/*
======================================================================
pointSearch()

Find the point with a given position.  Binary search gets us in the
neighborhood.  We then find the subarray of points with equal values
in the odb->vsortkey coordinate and linear search that subarray for a
complete match with pos[].  Returns the point index if a match is
found, otherwise returns -1.

If more than one point has the position, this returns the first point
index in the vsort[] array.
====================================================================== */

int pointSearch( ObjectDB *odb, float pos[ 3 ] )
{
   int lt = 0, rt = ARRAYSIZE - 1, x, key = KEY, ipos = IPOS;

   while ( rt >= lt ) {
      x = ( lt + rt ) / 2;
      if ( pos[ key ] < SORTVAL( x )) rt = x - 1; else lt = x + 1;
      if ( pos[ key ] == SORTVAL( x )) {
         lt = rt = x;
         while ( lt > 0 && pos[ key ] == SORTVAL( lt - 1 )) --lt;
         while ( rt < ARRAYSIZE - 1 && pos[ key ] == SORTVAL( rt + 1 )) ++rt;
         for ( x = lt; x <= rt; x++ )
            if ( pos[ 0 ] == COORD( x, 0 ) &&
                 pos[ 1 ] == COORD( x, 1 ) &&
                 pos[ 2 ] == COORD( x, 2 )) return ARRAY( x );
         return -1;
      }
   }
   return -1;
}

#pragma warning (default:4995)