#ifndef __GSIDEBUG_H__
#define __GSIDEBUG_H__
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  Advanced debug logging for GameSpy SDKs
//
//  Usage:
//      1) #define GSI_COMMON_DEBUG to enable debug output
//      2) Set target output (file or console or custom func)
//		3) Use Debug macros to log output
//
//  Todo:
//      Allow user to specify IP to send debug output to (remote log for PS2)
//#include "nonport.h"
#include <stdarg.h>

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Input levels (text is reported at one of these levels)
typedef gsi_u8 GSIDebugLevel;
#define GSIDebugLevel_HotError   (GSIDebugLevel)(1<<0)   //  1 Unexpected Error
#define GSIDebugLevel_WarmError  (GSIDebugLevel)(1<<1)   //  2 Expected Error
#define GSIDebugLevel_Warning    (GSIDebugLevel)(1<<2)   //  4 Warnings and Errors
#define GSIDebugLevel_Notice     (GSIDebugLevel)(1<<3)   //  8 Usefull debug info
#define GSIDebugLevel_Comment    (GSIDebugLevel)(1<<4)   // 16 Debug spam
#define GSIDebugLevel_RawDump    (GSIDebugLevel)(1<<5)   // 32 e.g. MemoryBuffer
#define GSIDebugLevel_StackTrace (GSIDebugLevel)(1<<6)   // 64 Important function entries
// add new ones here (update string table in gsiDebug.c!)
#define GSIDebugLevel_Count      7   // 7 reporting levels

// Output levels (a mask for the levels you want to receive)
// (update string table in gsiDebug.c!)
#define GSIDebugLevel_None       (GSIDebugLevel)(0)    //    No output
#define GSIDebugLevel_Normal     (GSIDebugLevel)(0x07) //    Warnings and above
#define GSIDebugLevel_Debug      (GSIDebugLevel)(0x0F) //    Notice and above
#define GSIDebugLevel_Verbose    (GSIDebugLevel)(0x1F) //    Comment and above
#define GSIDebugLevel_Hardcore   (GSIDebugLevel)(0xFF) //    Recv all


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Output types
typedef enum 
{
	GSIDebugType_Network,  // Network activity
	GSIDebugType_File,     // File output
	GSIDebugType_Memory,   // Memory allocations
	GSIDebugType_State,    // State update
	GSIDebugType_Misc,     // None of the above
	// add new ones here (update string table in gsiDebug.c!)

	GSIDebugType_Count,
	GSIDebugType_All = GSIDebugType_Count
} GSIDebugType;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Debug categories (SDKs)
typedef enum
{
	GSIDebugCat_App,
	GSIDebugCat_GP,
	GSIDebugCat_Peer,
	GSIDebugCat_QR2,
	GSIDebugCat_SB,
	GSIDebugCat_Voice,
	GSIDebugCat_AD,
	GSIDebugCat_NatNeg,
	GSIDebugCat_HTTP,
	GSIDebugCat_CDKey,
	// Add new ones here (update string table in gsiDebug.c!)


	GSIDebugCat_Common, // Common should be last to prevent display weirdness
	                    // resulting from initialization order
	GSIDebugCat_Count,
	GSIDebugCat_All = GSIDebugCat_Count
} GSIDebugCategory;

extern char* gGSIDebugCatStrings[GSIDebugCat_Count];
extern char* gGSIDebugTypeStrings[GSIDebugType_Count];
extern char* gGSIDebugLevelStrings[GSIDebugLevel_Count];

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Only include static data and functions if GSI_COMMON_DEBUG is defined
#ifndef GSI_COMMON_DEBUG
	// not using GSI debug!  Define functions to <blank>
	// (put these here so VisualAssist will resolve the definitions below)
	#if !defined(_WIN32) && !defined(__MWERKS__)
		// WIN32 doesn't like "..." in a macro
		#define gsDebugFormat(c,t,l,f,...)
		#define gsDebugVaList(c,t,l,f,v)
		#define gsDebugBinary(c,t,l,b,n)
		#define gsSetDebugLevel(c,t,l)
		#define gsSetDebugFile(f)
		#define gsOpenDebugFile(f)
		#define gsGetDebugFile
		#define gsSetDebugCallback(c)
	#elif defined(_NITRO)
		#define gsDebugFormat(...)
		#define gsDebugVaList(c,t,l,f,v)
		#define gsDebugBinary(c,t,l,b,n)
		#define gsSetDebugLevel(c,t,l)
		#define gsSetDebugFile(f)
		#define gsOpenDebugFile(f)
		#define gsGetDebugFile
		#define gsSetDebugCallback(c)
	#else
		#define gsDebugFormat
		#define gsDebugVaList
		#define gsDebugBinary
		#define gsSetDebugLevel
		#define gsSetDebugFile
		#define gsOpenDebugFile
		#define gsGetDebugFile
		#define gsSetDebugCallback
	#endif
#else


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// User supplied debug function, will receive debug text
typedef void (*GSIDebugCallback)(GSIDebugCategory,GSIDebugType,GSIDebugLevel,
                                   const char*, va_list);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Global debug instance
typedef struct GSIDebugInstance
{
#if !defined(_NITRO)
	FILE* mGSIDebugFile;
#endif
	GSIDebugCallback mDebugCallback;
	gsi_i32 mInitialized;

#if !defined(GSI_NO_THREADS)
	GSICriticalSection mDebugCrit;
#endif

	GSIDebugLevel mGSIDebugLevel[GSIDebugCat_Count][GSIDebugType_Count];
} GSIDebugInstance;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Logging functions
void gsDebugFormat(GSIDebugCategory theCat, GSIDebugType theType, 
             GSIDebugLevel theLevel, const char* theTokenStr, ...);  

void gsDebugVaList(GSIDebugCategory theCat, GSIDebugType theType, 
             GSIDebugLevel theLevel, const char* theTokenStr, 
			 va_list theParams);  

void gsDebugBinary(GSIDebugCategory theCat, GSIDebugType theType,
             GSIDebugLevel theLevel, const char* theBuffer, gsi_i32 theLength);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Output functions
void  gsSetDebugLevel(GSIDebugCategory theCat, GSIDebugType theType, 
					  GSIDebugLevel theLevel);

#if !defined(_NITRO)

// Set the output file (NULL for no file)
void  gsSetDebugFile(FILE* theFile);

// Open and set the debug file
FILE* gsOpenDebugFile(const char* theFileName);

// Retrieve the debug file
FILE* gsGetDebugFile();

#endif

// Set a callback to be triggered with debug output
void  gsSetDebugCallback(GSIDebugCallback theCallback);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // GSI_COMMON_DEBUG

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif // __GSIDEBUG_H__
