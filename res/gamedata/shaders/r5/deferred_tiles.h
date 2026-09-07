#ifndef DEFERRED_TILES_H
#define DEFERRED_TILES_H

#define TILE_SIZE 8
#define TILE_BIT_GEOMETRY 1u
#define TILE_BIT_SUN_MIXED 2u
#define TILE_BIT_LIGHTS 4u

cbuffer TileParams : register(b5)
{
    uint tilesX;
    uint tilesY;
    uint maxTiles;
    uint listBase;
    uint forceMixed;
    uint3 tilePad;
};

#endif
