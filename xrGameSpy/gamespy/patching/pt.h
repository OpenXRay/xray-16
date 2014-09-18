/*
GameSpy PT SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

18002 Skypark Circle
Irvine, California 92614
Tel: 949.798.4200
Fax: 949.798.4299
*/

/**************************************
** GameSpy Patching and Tracking SDK **
**************************************/

#ifndef _PT_H_
#define _PT_H_

#include "../common/gsCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********
** TYPES **
**********/

// Boolean.
///////////
typedef int PTBool;
#define PTFalse 0
#define PTTrue  1

/**************
** FUNCTIONS **
**************/
#ifndef GSI_UNICODE
#define ptCheckForPatch		ptCheckForPatchA
#define ptTrackUsage		ptTrackUsageA
#define ptCheckForPatchAndTrackUsage	ptCheckForPatchAndTrackUsageA
#define ptCreateCheckPatchTrackUsageReq       ptCreateCheckPatchTrackUsageReqA
#else
#define ptCheckForPatch		ptCheckForPatchW
#define ptTrackUsage		ptTrackUsageW
#define ptCheckForPatchAndTrackUsage	ptCheckForPatchAndTrackUsageW
#define ptCreateCheckPatchTrackUsageReq ptCreateCheckPatchTrackUsageReqW
#endif

// This callback gets called when a patch is being checked for
// with either ptCheckForPatch or ptCheckForPatchAndTrackUsage.
// If a patch is available, and the fileID is not 0, then
// ptLookupFilePlanetInfo can be used to find download sites.
///////////////////////////////////////////////////////////////
typedef void (* ptPatchCallback)
(
	PTBool available,         // If PTTrue, a patch is available.
	PTBool mandatory,         // If PTTrue, this patch is flagged as being mandatory.
	const gsi_char * versionName, // The display name for the new version.
	int fileID,               // The FilePlanet fileID for the patch, or 0.
	const gsi_char * downloadURL, // A URL to download the patch from, or NULL.
	void * param              // User-data passed originally passed to the function.
);

// Check for a patch for the current version and particular
// distribution of the product. If this function does not return
// PTFalse, then the callback will be called with info on a
// possible patch to a newer version.
///////////////////////////////////////////////////////////////
PTBool ptCheckForPatch
(
	int productID,                // The ID of this product.
	const gsi_char * versionUniqueID, // A string uniquely identifying this version.
	int distributionID,           // The distribution ID for this version.  Can be 0.
	ptPatchCallback callback,     // The callback to call with the patch info.
	PTBool blocking,              // If PTTrue, don't return until after the callback is called.
	void * param                  // User-data to pass to the callback.
);

// Used to track usage of a product, based on version and distribution.
// If PTFalse is returned, there was an error tracking usage.
///////////////////////////////////////////////////////////////////////
PTBool ptTrackUsage
(
	int userID,                   // The GP userID of the user who is using the product.  Can be 0.
	int productID,                // The ID of this product.
	const gsi_char * versionUniqueID, // A string uniquely identifying this version.
	int distributionID,           // The distribution ID for this version.  Can be 0.
	PTBool blocking
);

// This does the same thing as both ptCheckForPatch and
// ptTrackUsage in one call.
///////////////////////////////////////////////////////
PTBool ptCheckForPatchAndTrackUsage
(
	int userID,                   // The GP userID of the user who is using the product.  Can be 0.
	int productID,                // The ID of this product.
	const gsi_char * versionUniqueID, // A string uniquely identifying this version.
	int distributionID,           // The distribution ID for this version.  Can be 0.
	ptPatchCallback callback,     // The callback to call with the patch info.
	PTBool blocking,              // If PTTrue, don't return until after the callback is called.
	void * param                  // User-data to pass to the callback.
);

// This function is similar to the function ptCheckForPatchAndTrackUsage
// except that it returns a handle that can be used to call ghttpRequestThink
// or a failure of -1
//////////////////////////////////////////////////////
int ptCreateCheckPatchTrackUsageReq
(
	int userID,                   // The GP userID of the user who is using the product.  Can be 0.
	int productID,                // The ID of this product.
	const gsi_char * versionUniqueID, // A string uniquely identifying this version.
	int distributionID,           // The distribution ID for this version.  Can be 0.
	ptPatchCallback callback,     // The callback to call with the patch info.
	PTBool blocking,              // If PTTrue, don't return until after the callback is called.
	void * param                  // User-data to pass to the callback.
 );

// This callback gets called when looking up info on a
// FilePlanet file. If the file is found, it provides a
// text description, size, and a list of download sites.
////////////////////////////////////////////////////////
typedef void (* ptFilePlanetInfoCallback)
(
	int fileID,                // The ID of the file for which info was looked up.
	PTBool found,              // PTTrue if the file was found.
	const gsi_char * description,  // A user-readable description of the file.
	const gsi_char * size,         // A user-readable size of the file.
	int numMirrors,            // The number of mirrors found for this file.
	const gsi_char ** mirrorNames, // The names of the mirrors.
	const gsi_char ** mirrorURLs,  // The URLS for downloading the files.
	void * param               // User-data passed originally passed to the function.
);

// 9/7/2004 (xgd) ptLookupFilePlanetInfo() deprecated; per case 2724.
//
// Use this function to lookup info on a fileplanet file, by ID.
// This can be used to find a list of download sites for a patch
// based on the fileID passed to ptPatchCallback.  If PTFalse is
// returned, then there was an error and the callback will not
// be called.
////////////////////////////////////////////////////////////////
PTBool ptLookupFilePlanetInfo
(
	int fileID,                        // The ID of the file to find info on.
	ptFilePlanetInfoCallback callback, // The callback to call with the patch info.
	PTBool blocking,                   // If PTTrue, don't return until after the callback is called.
	void * param                       // User-data to pass to the callback.
);

#ifdef __cplusplus
}
#endif

#endif
