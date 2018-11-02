#include "stdafx.h"
#include "NvTriStripObjects.h"
#include "NvTriStrip.h"

#pragma warning(disable : 4018)

////////////////////////////////////////////////////////////////////////////////////////
// private data
static unsigned int cacheSize = CACHESIZE_GEFORCE1_2;
static bool bStitchStrips = true;
static unsigned int minStripSize = 0;
static bool bListsOnly = false;

////////////////////////////////////////////////////////////////////////////////////////
// SetListsOnly()
//
// If set to true, will return an optimized list, with no strips at all.
//
// Default value: false
//
void SetListsOnly(const bool _bListsOnly) { bListsOnly = _bListsOnly; }
////////////////////////////////////////////////////////////////////////////////////////
// SetCacheSize()
//
// Sets the cache size which the stripfier uses to optimize the data.
// Controls the length of the generated individual strips.
// This is the "actual" cache size, so 24 for GeForce3 and 16 for GeForce1/2
// You may want to play around with this number to tweak performance.
//
// Default value: 16
//
void SetCacheSize(const unsigned int _cacheSize) { cacheSize = _cacheSize; }
////////////////////////////////////////////////////////////////////////////////////////
// SetStitchStrips()
//
// bool to indicate whether to stitch together strips into one huge strip or not.
// If set to true, you'll get back one huge strip stitched together using degenerate
//  triangles.
// If set to false, you'll get back a large number of separate strips.
//
// Default value: true
//
void SetStitchStrips(const bool _bStitchStrips) { bStitchStrips = _bStitchStrips; }
////////////////////////////////////////////////////////////////////////////////////////
// SetMinStripSize()
//
// Sets the minimum acceptable size for a strip, in triangles.
// All strips generated which are shorter than this will be thrown into one big, separate list.
//
// Default value: 0
//
void SetMinStripSize(const unsigned int _minStripSize) { minStripSize = _minStripSize; }
////////////////////////////////////////////////////////////////////////////////////////
// GenerateStrips()
//
// in_indices: input index list, the indices you would use to render
// in_numIndices: number of entries in in_indices
// primGroups: array of optimized/stripified PrimitiveGroups
// numGroups: number of groups returned
//
// Be sure to call xr_free on the returned primGroups to avoid leaking mem
//
void GenerateStrips(const u16* in_indices, const s32 in_numIndices, xr_vector<PrimitiveGroup>& primGroups)
{
    // put data in format that the stripifier likes
    WordVec tempIndices;
    tempIndices.resize(in_numIndices);
    for (int i = 0; i < in_numIndices; i++)
        tempIndices[i] = in_indices[i];
    NvStripInfoVec tempStrips;
    NvFaceInfoVec tempFaces;

    NvStripifier stripifier;

    // do actual stripification
    stripifier.Stripify(tempIndices, cacheSize, minStripSize, tempStrips, tempFaces);

    // stitch strips together
    IntVec stripIndices;
    unsigned int numSeparateStrips = 0;

    if (bListsOnly)
    {
        // if we're outputting only lists, we're done
        primGroups.resize(1);

        // count the total number of indices
        unsigned int numIndices = 0;
        for (int i = 0; i < tempStrips.size(); i++)
        {
            numIndices += tempStrips[i]->m_faces.size() * 3;
        }

        // add in the list
        numIndices += tempFaces.size() * 3;

        primGroups[0].type = PT_LIST;
        primGroups[0].numIndices = numIndices;
        primGroups[0].indices = xr_alloc<u16>(numIndices);

        // do strips
        unsigned int indexCtr = 0;
        for (u32 i = 0; i < tempStrips.size(); i++)
        {
            for (int j = 0; j < tempStrips[i]->m_faces.size(); j++)
            {
                primGroups[0].indices[indexCtr++] = u16(tempStrips[i]->m_faces[j]->m_v0);
                primGroups[0].indices[indexCtr++] = u16(tempStrips[i]->m_faces[j]->m_v1);
                primGroups[0].indices[indexCtr++] = u16(tempStrips[i]->m_faces[j]->m_v2);
            }
        }

        // do lists
        for (u32 i = 0; i < tempFaces.size(); i++)
        {
            primGroups[0].indices[indexCtr++] = u16(tempFaces[i]->m_v0);
            primGroups[0].indices[indexCtr++] = u16(tempFaces[i]->m_v1);
            primGroups[0].indices[indexCtr++] = u16(tempFaces[i]->m_v2);
        }
    }
    else
    {
        stripifier.CreateStrips(tempStrips, stripIndices, bStitchStrips, numSeparateStrips);

        // if we're stitching strips together, we better get back only one strip from CreateStrips()
        R_ASSERT((bStitchStrips && (numSeparateStrips == 1)) || !bStitchStrips);

        // convert to output format
        int numGroups = u16(numSeparateStrips); // for the strips
        if (tempFaces.size() != 0)
            numGroups++; // we've got a list as well, increment
        primGroups.resize(numGroups);

        // first, the strips
        int startingLoc = 0;
        int stripCtr;
        for (stripCtr = 0; stripCtr < numSeparateStrips; stripCtr++)
        {
            int stripLength = 0;
            if (numSeparateStrips != 1)
            {
                // if we've got multiple strips, we need to figure out the correct length
                int i;
                for (i = startingLoc; i < stripIndices.size(); i++)
                {
                    if (stripIndices[i] == -1)
                        break;
                }

                stripLength = i - startingLoc;
            }
            else
                stripLength = stripIndices.size();

            primGroups[stripCtr].type = PT_STRIP;
            primGroups[stripCtr].indices = xr_alloc<u16>(stripLength);
            primGroups[stripCtr].numIndices = stripLength;

            int indexCtr = 0;
            for (int i = startingLoc; i < stripLength + startingLoc; i++)
                primGroups[stripCtr].indices[indexCtr++] = u16(stripIndices[i]);

            startingLoc += stripLength + 1; // we add 1 to account for the -1 separating strips
        }

        // next, the list
        if (tempFaces.size() != 0)
        {
            int faceGroupLoc = numGroups - 1; // the face group is the last one
            primGroups[faceGroupLoc].type = PT_LIST;
            primGroups[faceGroupLoc].indices = xr_alloc<u16>(tempFaces.size() * 3);
            primGroups[faceGroupLoc].numIndices = tempFaces.size() * 3;
            int indexCtr = 0;
            for (int i = 0; i < tempFaces.size(); i++)
            {
                primGroups[faceGroupLoc].indices[indexCtr++] = u16(tempFaces[i]->m_v0);
                primGroups[faceGroupLoc].indices[indexCtr++] = u16(tempFaces[i]->m_v1);
                primGroups[faceGroupLoc].indices[indexCtr++] = u16(tempFaces[i]->m_v2);
            }
        }
    }

    // clean up everything

    //_delete strips
    for (u32 i = 0; i < tempStrips.size(); i++)
    {
        for (int j = 0; j < tempStrips[i]->m_faces.size(); j++)
        {
            xr_delete(tempStrips[i]->m_faces[j]);
        }
        xr_delete(tempStrips[i]);
    }

    //_delete faces
    for (u32 i = 0; i < tempFaces.size(); i++)
    {
        xr_delete(tempFaces[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// RemapIndices()
//
// Function to remap your indices to improve spatial locality in your vertex buffer.
//
// in_primGroups: array of PrimitiveGroups you want remapped
// numGroups: number of entries in in_primGroups
// numVerts: number of vertices in your vertex buffer, also can be thought of as the range
//  of acceptable values for indices in your primitive groups.
// remappedGroups: array of remapped PrimitiveGroups
//
// Note that, according to the remapping handed back to you, you must reorder your
//  vertex buffer.
//
void RemapIndices(
    const xr_vector<PrimitiveGroup>& in_primGroups, const u16 numVerts, xr_vector<PrimitiveGroup>& remappedGroups)
{
    int numGroups = in_primGroups.size();
    remappedGroups.resize(numGroups);

    // caches oldIndex --> newIndex conversion
    int* indexCache;
    indexCache = xr_alloc<int>(numVerts);
    FillMemory(indexCache, sizeof(int) * numVerts, -1);

    // loop over primitive groups
    unsigned int indexCtr = 0;
    for (int i = 0; i < numGroups; i++)
    {
        unsigned int numIndices = in_primGroups[i].numIndices;

        // init remapped group
        remappedGroups[i].type = in_primGroups[i].type;
        remappedGroups[i].numIndices = numIndices;
        remappedGroups[i].indices = xr_alloc<u16>(numIndices);

        for (int j = 0; j < numIndices; j++)
        {
            int cachedIndex = indexCache[in_primGroups[i].indices[j]];
            if (cachedIndex == -1) // we haven't seen this index before
            {
                // point to "last" vertex in VB
                remappedGroups[i].indices[j] = u16(indexCtr);

                // add to index cache, increment
                indexCache[in_primGroups[i].indices[j]] = indexCtr++;
            }
            else
            {
                // we've seen this index before
                remappedGroups[i].indices[j] = u16(cachedIndex);
            }
        }
    }

    xr_free(indexCache);
}
