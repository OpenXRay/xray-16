/*
======================================================================
objectdb.h

Typedefs and function prototypes for an object database.
====================================================================== */

#ifndef OBJECTDB_H
#define OBJECTDB_H

#include <lwserver.h>
#include <lwmeshes.h>
#include <lwsurf.h>
#include <stdio.h>


typedef struct st_DBVMapVal {
   char        *name;            // name
   LWID         type;            // type of data (e.g. UVs, weights)
   int          dim;             // dimension (number of values per point)
   int          nverts;          // number of vertices
   int         *vindex;          // array of vertex indexes
   int         *vdpol;          // -1 - point map   0..n-polymap(polygon number)
   float      **val;             // 2D array of vmap values
} DBVMap;

typedef struct st_DBVMapPt {
   DBVMap      *vmap;            // where the values are stored
   int          index;           // offset into the vmap value arrays
} DBVMapPt;

typedef struct st_DBPoint {
   LWPntID      id;              // Layout point ID
   LWFVector    pos[ 2 ];        // initial and final position
   int          npols;           // number of polygons sharing the point
   int         *pol;             // array of polygon indexes
   int          nvmaps;          // number of vmap values
   DBVMapPt    *vm;              // array of vmap values
} DBPoint;

typedef struct st_DBPolVert {
   int          index;           // index into the point array
   LWFVector    norm[ 2 ];       // initial and final normals
   int			nvmaps;
   DBVMapPt    *vm;			     // array of vmap references
} DBPolVert;

typedef struct st_DBPolygon {
   LWPolID      id;              // Layout polygon ID
   int          sindex;          // surface index
   LWFVector    norm[ 2 ];       // initial and final normals
   int          nverts;          // number of vertices
   DBPolVert   *v;               // vertex array
} DBPolygon;

typedef struct st_lwPlugin {
   struct st_lwPlugin *next, *prev;
   char          *ord;
   char          *name;
   int            flags;
   void          *data;
} lwPlugin;

typedef struct st_DBSurface {
   LWSurfaceID  id;              // surface ID
   char			*name;           // surface name
   char			textures[8][64]; // textures
   int			tex_cnt;
   float        colr[ 3 ];       // color
   float        lumi;            // luminosity
   float        diff;            // diffuse level
   float        spec;            // specularity
   float        refl;            // reflectivity
   float        tran;            // transparency
   float        trnl;            // translucency
   float        rind;            // index of refraction
   float        bump;            // bump
   float        glos;            // glossiness
   float        shrp;            // diffuse sharpness
   float        sman;            // max smoothing angle (radians)
   float        rsan;            // reflection seam angle
   float        tsan;            // refraction seam angle
   float        clrf;            // color filter
   float        clrh;            // color highlight
   float        adtr;            // additive transparency
   float        aval;            // alpha value
   float        gval;            // glow value
   float        lcol[ 3 ];       // line color
   float        lsiz;            // line size
   int          alph;            // alpha options
   int          rfop;            // reflection options
   int          trop;            // refraction options
   int          side;            // sidedness
   int          glow;            // glow
   int          line;            // render outlines
   LWImageID    rimg;            // reflection image
   LWImageID    timg;            // refraction image
   lwPlugin		*shader;         // linked list of shaders
   int			nshaders;
} DBSurface;

typedef struct st_ObjectDB {
   char        *filename;        // object filename
   LWItemID     id;              // Layout item ID
   int          npoints;         // number of points
   int          npolygons;       // number of polygons
   int          nsurfaces;       // number of surfaces
   int          nvertmaps;       // number of vertex maps
   DBPoint     *pt;              // point array
   int         *vsort;           // point indexes sorted by point position
   int          vsortkey;        // coordinate for position sort
   DBPolygon   *pol;             // polygon array
   DBSurface   *surf;            // surface array
   DBVMap      *vmap;            // vmap array
} ObjectDB;


int findVert( ObjectDB *odb, LWPntID id );

void getPolyNormals( ObjectDB *odb, int i );
void getVertNormals( ObjectDB *odb, int i );

void freeObjectDB( ObjectDB *odb );
ObjectDB *getObjectDB( LWItemID id, GlobalFunc *global );
int printObjectDB( ObjectDB *odb, FILE *fp, int c );

void freeObjectVMaps( ObjectDB *odb );
int getObjectVMaps( ObjectDB *odb, LWMeshInfo *mesh, GlobalFunc *global );

void freeObjectSurfs( ObjectDB *odb );
int getObjectSurfs( ObjectDB *odb, LWMeshInfo *mesh, GlobalFunc *global );

int initPointSearch( ObjectDB *odb, int ipos );
void freePointSearch( ObjectDB *odb );
int pointSearch( ObjectDB *odb, float pos[ 3 ] );


#endif
