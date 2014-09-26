#include "stdafx.h"
/*
======================================================================
objectdb.c

Functions for creating an object database.
====================================================================== */

#include "objectdb.h"
#include "vecmat.h"

#pragma warning (disable:4995)

/*
======================================================================
pntScan()

Point scan callback.  Insert the point ID into the point array in
ascending numerical order.  This takes only linear time if point IDs
arrive in order.
====================================================================== */

static int pntScan( ObjectDB *odb, LWPntID id )
{
   int j;

   j = odb->npoints;

   if ( j == 0 ) {
      odb->pt[ 0 ].id = id;
      ++odb->npoints;
      return 0;
   }

   while ( odb->pt[ j - 1 ].id > id ) {
      odb->pt[ j ].id = odb->pt[ j - 1 ].id;
      --j;
      if ( j == 0 ) break;
   }

   odb->pt[ j ].id = id;
   ++odb->npoints;

   return 0;
}


/*
======================================================================
polScan()

Polygon scan callback.  Just store the ID.
====================================================================== */

static int polScan( ObjectDB *odb, LWPolID id )
{
   odb->pol[ odb->npolygons ].id = id;
   ++odb->npolygons;
   return 0;
}


/*
======================================================================
findVert()

Binary search the point array and return the array index for the given
point ID.
====================================================================== */

int findVert( ObjectDB *odb, LWPntID id )
{
   int lt = 0, rt = odb->npoints - 1, x;

   while ( rt >= lt ) {
      x = ( lt + rt ) / 2;
      if ( id < odb->pt[ x ].id ) rt = x - 1; else lt = x + 1;
      if ( id == odb->pt[ x ].id ) return x;
   }
   return -1;
}


/*
======================================================================
getPolyNormals()

Calculate the polygon normals.  By convention, LW's polygon normals
are based on the first, second and last points in the vertex list.
The normal is the cross product of two vectors formed from these
points.  It's undefined for one- and two-point polygons.
====================================================================== */

void getPolyNormals( ObjectDB *odb, int i )
{
   int j, k;
   LWFVector p1, p2, pn, v1, v2;

   for ( j = 0; j < odb->npolygons; j++ ) {
      if ( odb->pol[ j ].nverts < 3 ) continue;
      for ( k = 0; k < 3; k++ ) {
         p1[ k ] = odb->pt[ odb->pol[ j ].v[ 0 ].index ].pos[ i ][ k ];
         p2[ k ] = odb->pt[ odb->pol[ j ].v[ 1 ].index ].pos[ i ][ k ];
         pn[ k ] = odb->pt[ odb->pol[ j ].v[
            odb->pol[ j ].nverts - 1 ].index ].pos[ i ][ k ];
      }

      for ( k = 0; k < 3; k++ ) {
         v1[ k ] = p2[ k ] - p1[ k ];
         v2[ k ] = pn[ k ] - p1[ k ];
      }

      cross( v1, v2, odb->pol[ j ].norm[ i ] );
      normalize( odb->pol[ j ].norm[ i ] );
   }
}


/*
======================================================================
getVertNormals()

Calculate the vertex normals.  For each polygon vertex, sum the
normals of the polygons that share the point.  If the normals of the
current and adjacent polygons form an angle greater than the max
smoothing angle for the current polygon's surface, the normal of the
adjacent polygon is excluded from the sum.
====================================================================== */

void getVertNormals( ObjectDB *odb, int i )
{
   int j, k, n, g, h, p;
   float a;

   for ( j = 0; j < odb->npolygons; j++ ) {
      for ( n = 0; n < odb->pol[ j ].nverts; n++ ) {
         for ( k = 0; k < 3; k++ )
            odb->pol[ j ].v[ n ].norm[ i ][ k ]
               = odb->pol[ j ].norm[ i ][ k ];

         if ( odb->surf[ odb->pol[ j ].sindex ].sman <= 0 ) continue;

         p = odb->pol[ j ].v[ n ].index;

         for ( g = 0; g < odb->pt[ p ].npols; g++ ) {
            h = odb->pt[ p ].pol[ g ];
            if ( h == j ) continue;

            a = vecangle( odb->pol[ j ].norm[ i ], odb->pol[ h ].norm[ i ] );
            if ( a > odb->surf[ odb->pol[ j ].sindex ].sman ) continue;

            for ( k = 0; k < 3; k++ )
               odb->pol[ j ].v[ n ].norm[ i ][ k ]
                  += odb->pol[ h ].norm[ i ][ k ];
         }

         normalize( odb->pol[ j ].v[ n ].norm[ i ] );
      }
   }
}


/*
======================================================================
freeObjectDB()

Free an ObjectDB created by getObjectDB().
====================================================================== */

void freeObjectDB( ObjectDB *odb )
{
   int i;

   if ( odb ) {
      freeObjectVMaps( odb );
      freeObjectSurfs( odb );
      freePointSearch( odb );

      if ( odb->pt ) {
         for ( i = 0; i < odb->npoints; i++ )
            if ( odb->pt[ i ].pol )
               free( odb->pt[ i ].pol );
         free( odb->pt );
      }
      if ( odb->pol ) {
         for ( i = 0; i < odb->npolygons; i++ )
            if ( odb->pol[ i ].v )
               free( odb->pol[ i ].v );
         free( odb->pol );
      }
      free( odb );
   }
}


/*
======================================================================
getObjectDB()

Create an ObjectDB for an object.
====================================================================== */

ObjectDB *getObjectDB( LWItemID id, GlobalFunc *global )
{
   LWObjectInfo *objinfo;
   LWMeshInfo *mesh;
   LWPntID ptid;
   ObjectDB *odb;
   const char *name;
   int npts, npols, nverts, i, j, k, ok = 0;


   /* get the object info global */

   objinfo = (st_LWObjectInfo*)global( LWOBJECTINFO_GLOBAL, GFUSE_TRANSIENT );
   if ( !objinfo ) return NULL;

   /* get the mesh info for the object */

   mesh = objinfo->meshInfo( id, 1 );
   if ( !mesh ) return NULL;

   /* alloc the object database */

   odb = (st_ObjectDB*)calloc( 1, sizeof( ObjectDB ));
   if ( !odb ) goto Finish;

   odb->id = id;
   name = objinfo->filename( id );
   odb->filename = (char*)malloc( xr_strlen( name ) + 1 );
   if ( !odb->filename ) goto Finish;
   strcpy( odb->filename, name );

   /* alloc and init the points array */

   npts = mesh->numPoints( mesh );
   odb->pt = (st_DBPoint*)calloc( npts, sizeof( DBPoint ));
   if ( !odb->pt ) goto Finish;

   if ( mesh->scanPoints( mesh, (int (__cdecl *)(void *,struct st_GCoreVertex *))pntScan, odb ))
      goto Finish;

   /* alloc and init the polygons array */

   npols = mesh->numPolygons( mesh );
   odb->pol = (st_DBPolygon*)calloc( npols, sizeof( DBPolygon ));
   if ( !odb->pol ) goto Finish;

   if ( mesh->scanPolys( mesh, (int (__cdecl *)(void *,struct st_GCorePolygon *))polScan, odb ))
      goto Finish;

   /* get the vertices of each polygon */

   for ( i = 0; i < npols; i++ ) {
      nverts = mesh->polSize( mesh, odb->pol[ i ].id );
      odb->pol[ i ].v = (st_DBPolVert*)calloc( nverts, sizeof( DBPolVert ));
      if ( !odb->pol[ i ].v ) goto Finish;
      odb->pol[ i ].nverts = nverts;
      for ( j = 0; j < nverts; j++ ) {
         ptid = mesh->polVertex( mesh, odb->pol[ i ].id, j );
         odb->pol[ i ].v[ j ].index = findVert( odb, ptid );
      }
   }

   /* count the number of polygons per point */

   for ( i = 0; i < npols; i++ )
      for ( j = 0; j < odb->pol[ i ].nverts; j++ )
         ++odb->pt[ odb->pol[ i ].v[ j ].index ].npols;

   /* alloc per-point polygon arrays */

   for ( i = 0; i < npts; i++ ) {
      if ( odb->pt[ i ].npols == 0 ) continue;
      odb->pt[ i ].pol = (int*)calloc( odb->pt[ i ].npols, sizeof( int ));
      if ( !odb->pt[ i ].pol ) goto Finish;
      odb->pt[ i ].npols = 0;
   }

   /* fill in polygon array for each point */

   for ( i = 0; i < npols; i++ ) {
      for ( j = 0; j < odb->pol[ i ].nverts; j++ ) {
         k = odb->pol[ i ].v[ j ].index;
         odb->pt[ k ].pol[ odb->pt[ k ].npols ] = i;
         ++odb->pt[ k ].npols;
      }
   }

   /* get the base position of each point */

   for ( i = 0; i < npts; i++ )
      mesh->pntBasePos( mesh, odb->pt[ i ].id, odb->pt[ i ].pos[ 0 ] );

   /* init the point search array */
   /* do this here if you need to search by base pos, or later if you
      need to search by final pos */

   // if ( !initPointSearch( odb, 0 ))
   //    goto Finish;

   /* calculate the base normal of each polygon */

   getPolyNormals( odb, 0 );

   /* get the vmaps */

   if ( !getObjectVMaps( odb, mesh, global ))
      goto Finish;

   /* get the surfaces */

   if ( !getObjectSurfs( odb, mesh, global ))
      goto Finish;

   /* calculate initial vertex normals */

   getVertNormals( odb, 0 );

   /* done */

   ok = 1;

Finish:
   if ( mesh->destroy )
      mesh->destroy( mesh );

   if ( !ok ) {
      freeObjectDB( odb );
      return NULL;
   }

   return odb;
}


/*
======================================================================
printObjectDB()

Print the information in an object database to a file.  This is mostly
for debugging and to illustrate how to reference the items in the
database.  The c argument is 0 for initial normals and point positions
and 1 for final.
====================================================================== */

int printObjectDB( ObjectDB *odb, FILE *fp, int c )
{
   DBVMap *vmap;
   char *tag;
   int i, j, k, n;

   fprintf( fp, "%08.8x %s\n\n", odb->id, odb->filename );

   fprintf( fp, "Points (%d)\n\n", odb->npoints );
   for ( i = 0; i < odb->npoints; i++ ) {
      fprintf( fp, "%08.8x  pos (%g, %g, %g)  npols %d:",
         odb->pt[ i ].id,
         odb->pt[ i ].pos[ c ][ 0 ],
         odb->pt[ i ].pos[ c ][ 1 ],
         odb->pt[ i ].pos[ c ][ 2 ],
         odb->pt[ i ].npols );
      for ( j = 0; j < odb->pt[ i ].npols; j++ )
         fprintf( fp, " %d", odb->pt[ i ].pol[ j ] );
      fprintf( fp, "\n" );

      for ( j = 0; j < odb->pt[ i ].nvmaps; j++ ) {
         vmap = odb->pt[ i ].vm[ j ].vmap;
         switch ( vmap->type ) {
            case LWVMAP_PICK:  tag = "PICK";  break;
            case LWVMAP_WGHT:  tag = "WGHT";  break;
            case LWVMAP_MNVW:  tag = "MNVW";  break;
            case LWVMAP_TXUV:  tag = "TXUV";  break;
            case LWVMAP_MORF:  tag = "MORF";  break;
            case LWVMAP_SPOT:  tag = "SPOT";  break;
            default:           tag = "Unknown type";  break;
         }
         fprintf( fp, "  %s %s:", tag, vmap->name );
         for ( k = 0; k < vmap->dim; k++ ) {
            n = odb->pt[ i ].vm[ j ].index;
            fprintf( fp, " %g", vmap->val[ k ][ n ] );
         }
         fprintf( fp, "\n" );
      }
   }

   /* test the point search */

   if ( initPointSearch( odb, c ))
      for ( i = 0; i < odb->npoints; i++ )
         if ( i != pointSearch( odb, odb->pt[ i ].pos[ c ] ))
            fprintf( fp, "Point search failed for point %d.\n", i );

   fprintf( fp, "\n\nPolygons (%d)\n\n", odb->npolygons );
   for ( i = 0; i < odb->npolygons; i++ ) {
      fprintf( fp, "%08.8x  surf %d  norm (%g, %g, %g)  nverts %d:\n",
         odb->pol[ i ].id,
         odb->pol[ i ].sindex,
         odb->pol[ i ].norm[ c ][ 0 ],
         odb->pol[ i ].norm[ c ][ 1 ],
         odb->pol[ i ].norm[ c ][ 2 ],
         odb->pol[ i ].nverts );
      for ( j = 0; j < odb->pol[ i ].nverts; j++ ) {
         fprintf( fp, "  vert %d  vnorm (%g, %g, %g)\n",
            odb->pol[ i ].v[ j ].index,
            odb->pol[ i ].v[ j ].norm[ c ][ 0 ],
            odb->pol[ i ].v[ j ].norm[ c ][ 1 ],
            odb->pol[ i ].v[ j ].norm[ c ][ 2 ] );
      }
   }

   fprintf( fp, "\n\nSurfaces (%d)\n", odb->nsurfaces );
   for ( i = 0; i < odb->nsurfaces; i++ ) {
      fprintf( fp, "\n%08.8X  \"%s\"\n",
         odb->surf[ i ].id,
         odb->surf[ i ].name );
      fprintf( fp, "  " SURF_COLR "  %g %g %g\n",
         odb->surf[ i ].colr[ 0 ],
         odb->surf[ i ].colr[ 1 ],
         odb->surf[ i ].colr[ 2 ] );
      fprintf( fp, "  " SURF_LUMI "  %g\n", odb->surf[ i ].lumi );
      fprintf( fp, "  " SURF_DIFF "  %g\n", odb->surf[ i ].diff );
      fprintf( fp, "  " SURF_SPEC "  %g\n", odb->surf[ i ].spec );
      fprintf( fp, "  " SURF_REFL "  %g\n", odb->surf[ i ].refl );
      fprintf( fp, "  " SURF_RFOP "  0x%X\n", odb->surf[ i ].rfop );
      fprintf( fp, "  " SURF_TRAN "  %g\n", odb->surf[ i ].tran );
      fprintf( fp, "  " SURF_TROP "  0x%X\n", odb->surf[ i ].trop );
      fprintf( fp, "  " SURF_TRNL "  %g\n", odb->surf[ i ].trnl );
      fprintf( fp, "  " SURF_RIND "  %g\n", odb->surf[ i ].rind );
      fprintf( fp, "  " SURF_BUMP "  %g\n", odb->surf[ i ].bump );
      fprintf( fp, "  " SURF_GLOS "  %g\n", odb->surf[ i ].glos );
      fprintf( fp, "  " SURF_SHRP "  %g\n", odb->surf[ i ].shrp );
      fprintf( fp, "  " SURF_SMAN "  %g\n", odb->surf[ i ].sman );
      fprintf( fp, "  " SURF_RSAN "  %g\n", odb->surf[ i ].rsan );
      fprintf( fp, "  " SURF_TSAN "  %g\n", odb->surf[ i ].tsan );
      fprintf( fp, "  " SURF_CLRF "  %g\n", odb->surf[ i ].clrf );
      fprintf( fp, "  " SURF_CLRH "  %g\n", odb->surf[ i ].clrh );
      fprintf( fp, "  " SURF_ADTR "  %g\n", odb->surf[ i ].adtr );
      fprintf( fp, "  " SURF_ALPH "  0x%X\n", odb->surf[ i ].alph );
      fprintf( fp, "  " SURF_AVAL "  %g\n", odb->surf[ i ].aval );
      fprintf( fp, "  " SURF_GLOW "  0x%X\n", odb->surf[ i ].glow );
      fprintf( fp, "  " SURF_GVAL "  %g\n", odb->surf[ i ].gval );
      fprintf( fp, "  " SURF_LINE "  0x%X\n", odb->surf[ i ].line );
      fprintf( fp, "  " SURF_LSIZ "  %g\n", odb->surf[ i ].lsiz );
      fprintf( fp, "  " SURF_LCOL "  %g %g %g\n",
         odb->surf[ i ].lcol[ 0 ],
         odb->surf[ i ].lcol[ 1 ],
         odb->surf[ i ].lcol[ 2 ] );
      fprintf( fp, "  " SURF_SIDE "  0x%X\n", odb->surf[ i ].side );
      fprintf( fp, "  " SURF_RIMG "  %08.8X\n", odb->surf[ i ].rimg );
      fprintf( fp, "  " SURF_TIMG "  %08.8X\n", odb->surf[ i ].timg );
   }

   fprintf( fp, "\n\nVertex Maps (%d)\n\n", odb->nvertmaps );
   for ( i = 0; i < odb->nvertmaps; i++ ) {
      tag = ( char * ) &odb->vmap[ i ].type;
      fprintf( fp, "%c%c%c%c \"%s\"  dim %d  nverts %d\n",
#ifdef _WIN32
         tag[ 3 ], tag[ 2 ], tag[ 1 ], tag[ 0 ],
#else
         tag[ 0 ], tag[ 1 ], tag[ 2 ], tag[ 3 ],
#endif
         odb->vmap[ i ].name,
         odb->vmap[ i ].dim,
         odb->vmap[ i ].nverts );
      for ( j = 0; j < odb->vmap[ i ].nverts; j++ ) {
         fprintf( fp, "  %d ", odb->vmap[ i ].vindex[ j ] );
         for ( k = 0; k < odb->vmap[ i ].dim; k++ )
            fprintf( fp, " %g", odb->vmap[ i ].val[ k ][ j ] );
         fprintf( fp, "\n" );
      }
   }

   return 1;
}


#pragma warning (default:4995)