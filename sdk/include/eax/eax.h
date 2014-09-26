/******************************************************************
*
*  EAX.H - DirectSound3D Environmental Audio Extensions version 2.0
*  Updated July 8, 1999
*
*******************************************************************
*/

#ifndef EAX_H_INCLUDED
#define EAX_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef OPENAL
	#include <dsound.h>
	/*
	* EAX Wrapper Interface (using Direct X 7) {4FF53B81-1CE0-11d3-AAB8-00A0C95949D5}
	*/
	DEFINE_GUID(CLSID_EAXDirectSound, 
		0x4ff53b81, 
		0x1ce0, 
		0x11d3,
		0xaa, 0xb8, 0x0, 0xa0, 0xc9, 0x59, 0x49, 0xd5);

	/*
	* EAX Wrapper Interface (using Direct X 8) {CA503B60-B176-11d4-A094-D0C0BF3A560C}
	*/
	DEFINE_GUID(CLSID_EAXDirectSound8, 
		0xca503b60,
		0xb176,
		0x11d4,
		0xa0, 0x94, 0xd0, 0xc0, 0xbf, 0x3a, 0x56, 0xc);

#ifdef DIRECTSOUND_VERSION
#if DIRECTSOUND_VERSION == 0x0900
		__declspec(dllimport) HRESULT WINAPI EAXDirectSoundCreate8(GUID*, LPDIRECTSOUND8*, IUnknown FAR *);
		typedef HRESULT (FAR PASCAL *LPEAXDIRECTSOUNDCREATE8)(GUID*, LPDIRECTSOUND8*, IUnknown FAR*);
#endif
#endif

	__declspec(dllimport) HRESULT WINAPI EAXDirectSoundCreate(GUID*, LPDIRECTSOUND*, IUnknown FAR *);
	typedef HRESULT (FAR PASCAL *LPEAXDIRECTSOUNDCREATE)(GUID*, LPDIRECTSOUND*, IUnknown FAR*);

#else
	#include <openal/al.h>

	#ifndef GUID_DEFINED
		#define GUID_DEFINED
		typedef struct _GUID
		{
			unsigned long Data1;
			unsigned short Data2;
			unsigned short Data3;
			unsigned char Data4[8];
		} GUID;
	#endif // !GUID_DEFINED

	#ifndef DEFINE_GUID
		#ifndef INITGUID
			#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
					extern const GUID FAR name
		#else
			#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
					extern const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
		#endif // INITGUID
	#endif // DEFINE_GUID

	/*
	* EAX OpenAL Extension {4FF53B81-1CE0-11d3-AAB8-00A0C95949D5}
	*/
	typedef ALenum (__cdecl *EAXSet)(const GUID*, ALuint, ALuint, ALvoid*, ALuint);
	typedef ALenum (__cdecl *EAXGet)(const GUID*, ALuint, ALuint, ALvoid*, ALuint);
#endif

#pragma pack(push, 4)

/*
* EAX 2.0 listener property set {0306A6A8-B224-11d2-99E5-0000E8D8C722}
*/
DEFINE_GUID(DSPROPSETID_EAX20_ListenerProperties, 
    0x306a6a8, 
    0xb224, 
    0x11d2, 
    0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22);

// For compatibility with future EAX versions:
#define DSPROPSETID_EAX_ListenerProperties DSPROPSETID_EAX20_ListenerProperties
#define DSPROPSETID_EAX_SourceProperties DSPROPSETID_EAX20_BufferProperties

typedef enum
{
    DSPROPERTY_EAXLISTENER_NONE,
    DSPROPERTY_EAXLISTENER_ALLPARAMETERS,
    DSPROPERTY_EAXLISTENER_ROOM,
    DSPROPERTY_EAXLISTENER_ROOMHF,
    DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAXLISTENER_DECAYTIME,
    DSPROPERTY_EAXLISTENER_DECAYHFRATIO,
    DSPROPERTY_EAXLISTENER_REFLECTIONS,
    DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY,
    DSPROPERTY_EAXLISTENER_REVERB,
    DSPROPERTY_EAXLISTENER_REVERBDELAY,
    DSPROPERTY_EAXLISTENER_ENVIRONMENT,
    DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE,
    DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
    DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF,
    DSPROPERTY_EAXLISTENER_FLAGS
} DSPROPERTY_EAX_LISTENERPROPERTY;
	
// OR these flags with property id
#define DSPROPERTY_EAXLISTENER_IMMEDIATE 0x00000000 // changes take effect immediately
#define DSPROPERTY_EAXLISTENER_DEFERRED  0x80000000 // changes take effect later
#define DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS (DSPROPERTY_EAXLISTENER_NONE | \
                                                       DSPROPERTY_EAXLISTENER_IMMEDIATE)

// Use this structure for DSPROPERTY_EAXLISTENER_ALLPARAMETERS
// - all levels are hundredths of decibels
// - all times are in seconds
// - the reference for high frequency controls is 5 kHz
//
// NOTE: This structure may change in future EAX versions.
//       It is recommended to initialize fields by name:
//              myListener.lRoom = -1000;
//              myListener.lRoomHF = -100;
//              ...
//              myListener.dwFlags = myFlags /* see EAXLISTENERFLAGS below */ ;
//       instead of:
//              myListener = { -1000, -100, ... , 0x00000009 };
//       If you want to save and load presets in binary form, you 
//       should define your own structure to insure future compatibility.
//
typedef struct _EAXLISTENERPROPERTIES
{
    long lRoom;                    // room effect level at low frequencies
    long lRoomHF;                  // room effect high-frequency level re. low frequency level
    float flRoomRolloffFactor;     // like DS3D flRolloffFactor but for room effect
    float flDecayTime;             // reverberation decay time at low frequencies
    float flDecayHFRatio;          // high-frequency to low-frequency decay time ratio
    long lReflections;             // early reflections level relative to room effect
    float flReflectionsDelay;      // initial reflection delay time
    long lReverb;                  // late reverberation level relative to room effect
    float flReverbDelay;           // late reverberation delay time relative to initial reflection
    unsigned long dwEnvironment;   // sets all listener properties
    float flEnvironmentSize;       // environment size in meters
    float flEnvironmentDiffusion;  // environment diffusion
    float flAirAbsorptionHF;       // change in level per meter at 5 kHz
    unsigned long dwFlags;         // modifies the behavior of properties
} EAXLISTENERPROPERTIES, *LPEAXLISTENERPROPERTIES;

// used by DSPROPERTY_EAXLISTENER_ENVIRONMENT
enum
{
    EAX_ENVIRONMENT_GENERIC,
    EAX_ENVIRONMENT_PADDEDCELL,
    EAX_ENVIRONMENT_ROOM,
    EAX_ENVIRONMENT_BATHROOM,
    EAX_ENVIRONMENT_LIVINGROOM,
    EAX_ENVIRONMENT_STONEROOM,
    EAX_ENVIRONMENT_AUDITORIUM,
    EAX_ENVIRONMENT_CONCERTHALL,
    EAX_ENVIRONMENT_CAVE,
    EAX_ENVIRONMENT_ARENA,
    EAX_ENVIRONMENT_HANGAR,
    EAX_ENVIRONMENT_CARPETEDHALLWAY,
    EAX_ENVIRONMENT_HALLWAY,
    EAX_ENVIRONMENT_STONECORRIDOR,
    EAX_ENVIRONMENT_ALLEY,
    EAX_ENVIRONMENT_FOREST,
    EAX_ENVIRONMENT_CITY,
    EAX_ENVIRONMENT_MOUNTAINS,
    EAX_ENVIRONMENT_QUARRY,
    EAX_ENVIRONMENT_PLAIN,
    EAX_ENVIRONMENT_PARKINGLOT,
    EAX_ENVIRONMENT_SEWERPIPE,
    EAX_ENVIRONMENT_UNDERWATER,
    EAX_ENVIRONMENT_DRUGGED,
    EAX_ENVIRONMENT_DIZZY,
    EAX_ENVIRONMENT_PSYCHOTIC,

    EAX_ENVIRONMENT_COUNT
};

// Used by DSPROPERTY_EAXLISTENER_FLAGS
//
// Note: The number and order of flags may change in future EAX versions.
//       It is recommended to use the flag defines as follows:
//              myFlags = EAXLISTENERFLAGS_DECAYTIMESCALE | EAXLISTENERFLAGS_REVERBSCALE;
//       instead of:
//              myFlags = 0x00000009;
//
// These flags determine what properties are affected by environment size.
#define EAXLISTENERFLAGS_DECAYTIMESCALE        0x00000001 // reverberation decay time
#define EAXLISTENERFLAGS_REFLECTIONSSCALE      0x00000002 // reflection level
#define EAXLISTENERFLAGS_REFLECTIONSDELAYSCALE 0x00000004 // initial reflection delay time
#define EAXLISTENERFLAGS_REVERBSCALE           0x00000008 // reflections level
#define EAXLISTENERFLAGS_REVERBDELAYSCALE      0x00000010 // late reverberation delay time

// This flag limits high-frequency decay time according to air absorption.
#define EAXLISTENERFLAGS_DECAYHFLIMIT          0x00000020
 
#define EAXLISTENERFLAGS_RESERVED              0xFFFFFFC0 // reserved future use

// property ranges and defaults:

#define EAXLISTENER_MINROOM                       (-10000)
#define EAXLISTENER_MAXROOM                       0
#define EAXLISTENER_DEFAULTROOM                   (-1000)

#define EAXLISTENER_MINROOMHF                     (-10000)
#define EAXLISTENER_MAXROOMHF                     0
#define EAXLISTENER_DEFAULTROOMHF                 (-100)

#define EAXLISTENER_MINROOMROLLOFFFACTOR          0.0f
#define EAXLISTENER_MAXROOMROLLOFFFACTOR          10.0f
#define EAXLISTENER_DEFAULTROOMROLLOFFFACTOR      0.0f

#define EAXLISTENER_MINDECAYTIME                  0.1f
#define EAXLISTENER_MAXDECAYTIME                  20.0f
#define EAXLISTENER_DEFAULTDECAYTIME              1.49f

#define EAXLISTENER_MINDECAYHFRATIO               0.1f
#define EAXLISTENER_MAXDECAYHFRATIO               2.0f
#define EAXLISTENER_DEFAULTDECAYHFRATIO           0.83f

#define EAXLISTENER_MINREFLECTIONS                (-10000)
#define EAXLISTENER_MAXREFLECTIONS                1000
#define EAXLISTENER_DEFAULTREFLECTIONS            (-2602)

#define EAXLISTENER_MINREFLECTIONSDELAY           0.0f
#define EAXLISTENER_MAXREFLECTIONSDELAY           0.3f
#define EAXLISTENER_DEFAULTREFLECTIONSDELAY       0.007f

#define EAXLISTENER_MINREVERB                     (-10000)
#define EAXLISTENER_MAXREVERB                     2000
#define EAXLISTENER_DEFAULTREVERB                 200

#define EAXLISTENER_MINREVERBDELAY                0.0f
#define EAXLISTENER_MAXREVERBDELAY                0.1f
#define EAXLISTENER_DEFAULTREVERBDELAY            0.011f

#define EAXLISTENER_MINENVIRONMENT                0
#define EAXLISTENER_MAXENVIRONMENT                (EAX_ENVIRONMENT_COUNT-1)
#define EAXLISTENER_DEFAULTENVIRONMENT            EAX_ENVIRONMENT_GENERIC

#define EAXLISTENER_MINENVIRONMENTSIZE            1.0f
#define EAXLISTENER_MAXENVIRONMENTSIZE            100.0f
#define EAXLISTENER_DEFAULTENVIRONMENTSIZE        7.5f

#define EAXLISTENER_MINENVIRONMENTDIFFUSION       0.0f
#define EAXLISTENER_MAXENVIRONMENTDIFFUSION       1.0f
#define EAXLISTENER_DEFAULTENVIRONMENTDIFFUSION   1.0f

#define EAXLISTENER_MINAIRABSORPTIONHF            (-100.0f)
#define EAXLISTENER_MAXAIRABSORPTIONHF            0.0f
#define EAXLISTENER_DEFAULTAIRABSORPTIONHF        (-5.0f)

#define EAXLISTENER_DEFAULTFLAGS                  (EAXLISTENERFLAGS_DECAYTIMESCALE |        \
                                                   EAXLISTENERFLAGS_REFLECTIONSSCALE |      \
                                                   EAXLISTENERFLAGS_REFLECTIONSDELAYSCALE | \
                                                   EAXLISTENERFLAGS_REVERBSCALE |           \
                                                   EAXLISTENERFLAGS_REVERBDELAYSCALE |      \
                                                   EAXLISTENERFLAGS_DECAYHFLIMIT)



/*
* EAX 2.0 buffer property set {0306A6A7-B224-11d2-99E5-0000E8D8C722}
*/
DEFINE_GUID(DSPROPSETID_EAX20_BufferProperties, 
    0x306a6a7, 
    0xb224, 
    0x11d2, 
    0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22);

// For compatibility with future EAX versions:
#define DSPROPSETID_EAX_BufferProperties DSPROPSETID_EAX20_BufferProperties

typedef enum
{
    DSPROPERTY_EAXBUFFER_NONE,
    DSPROPERTY_EAXBUFFER_ALLPARAMETERS,
    DSPROPERTY_EAXBUFFER_DIRECT,
    DSPROPERTY_EAXBUFFER_DIRECTHF,
    DSPROPERTY_EAXBUFFER_ROOM,
    DSPROPERTY_EAXBUFFER_ROOMHF, 
    DSPROPERTY_EAXBUFFER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAXBUFFER_OBSTRUCTION,
    DSPROPERTY_EAXBUFFER_OBSTRUCTIONLFRATIO,
    DSPROPERTY_EAXBUFFER_OCCLUSION, 
    DSPROPERTY_EAXBUFFER_OCCLUSIONLFRATIO,
    DSPROPERTY_EAXBUFFER_OCCLUSIONROOMRATIO,
    DSPROPERTY_EAXBUFFER_OUTSIDEVOLUMEHF,
    DSPROPERTY_EAXBUFFER_AIRABSORPTIONFACTOR,
    DSPROPERTY_EAXBUFFER_FLAGS
} DSPROPERTY_EAX_BUFFERPROPERTY;    

// OR these flags with property id
#define DSPROPERTY_EAXBUFFER_IMMEDIATE 0x00000000 // changes take effect immediately
#define DSPROPERTY_EAXBUFFER_DEFERRED  0x80000000 // changes take effect later
#define DSPROPERTY_EAXBUFFER_COMMITDEFERREDSETTINGS (DSPROPERTY_EAXBUFFER_NONE | \
                                                     DSPROPERTY_EAXBUFFER_IMMEDIATE)

// Use this structure for DSPROPERTY_EAXBUFFER_ALLPARAMETERS
// - all levels are hundredths of decibels
//
// NOTE: This structure may change in future EAX versions.
//       It is recommended to initialize fields by name:
//              myBuffer.lDirect = 0;
//              myBuffer.lDirectHF = -200;
//              ...
//              myBuffer.dwFlags = myFlags /* see EAXBUFFERFLAGS below */ ;
//       instead of:
//              myBuffer = { 0, -200, ... , 0x00000003 };
//
typedef struct _EAXBUFFERPROPERTIES
{
    long lDirect;                // direct path level
    long lDirectHF;              // direct path level at high frequencies
    long lRoom;                  // room effect level
    long lRoomHF;                // room effect level at high frequencies
    float flRoomRolloffFactor;   // like DS3D flRolloffFactor but for room effect
    long lObstruction;           // main obstruction control (attenuation at high frequencies) 
    float flObstructionLFRatio;  // obstruction low-frequency level re. main control
    long lOcclusion;             // main occlusion control (attenuation at high frequencies)
    float flOcclusionLFRatio;    // occlusion low-frequency level re. main control
    float flOcclusionRoomRatio;  // occlusion room effect level re. main control
    long lOutsideVolumeHF;       // outside sound cone level at high frequencies
    float flAirAbsorptionFactor; // multiplies DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF
    unsigned long dwFlags;       // modifies the behavior of properties
} EAXBUFFERPROPERTIES, *LPEAXBUFFERPROPERTIES;

// Used by DSPROPERTY_EAXBUFFER_FLAGS
//    TRUE:    value is computed automatically - property is an offset
//    FALSE:   value is used directly
//
// Note: The number and order of flags may change in future EAX versions.
//       To insure future compatibility, use flag defines as follows:
//              myFlags = EAXBUFFERFLAGS_DIRECTHFAUTO | EAXBUFFERFLAGS_ROOMAUTO;
//       instead of:
//              myFlags = 0x00000003;
//
#define EAXBUFFERFLAGS_DIRECTHFAUTO 0x00000001 // affects DSPROPERTY_EAXBUFFER_DIRECTHF
#define EAXBUFFERFLAGS_ROOMAUTO     0x00000002 // affects DSPROPERTY_EAXBUFFER_ROOM
#define EAXBUFFERFLAGS_ROOMHFAUTO   0x00000004 // affects DSPROPERTY_EAXBUFFER_ROOMHF

#define EAXBUFFERFLAGS_RESERVED     0xFFFFFFF8 // reserved future use

// property ranges and defaults:

#define EAXBUFFER_MINDIRECT                  (-10000)
#define EAXBUFFER_MAXDIRECT                  1000
#define EAXBUFFER_DEFAULTDIRECT              0

#define EAXBUFFER_MINDIRECTHF                (-10000)
#define EAXBUFFER_MAXDIRECTHF                0
#define EAXBUFFER_DEFAULTDIRECTHF            0

#define EAXBUFFER_MINROOM                    (-10000)
#define EAXBUFFER_MAXROOM                    1000
#define EAXBUFFER_DEFAULTROOM                0

#define EAXBUFFER_MINROOMHF                  (-10000)
#define EAXBUFFER_MAXROOMHF                  0
#define EAXBUFFER_DEFAULTROOMHF              0

#define EAXBUFFER_MINROOMROLLOFFFACTOR       0.0f
#define EAXBUFFER_MAXROOMROLLOFFFACTOR       10.f
#define EAXBUFFER_DEFAULTROOMROLLOFFFACTOR   0.0f

#define EAXBUFFER_MINOBSTRUCTION             (-10000)
#define EAXBUFFER_MAXOBSTRUCTION             0
#define EAXBUFFER_DEFAULTOBSTRUCTION         0

#define EAXBUFFER_MINOBSTRUCTIONLFRATIO      0.0f
#define EAXBUFFER_MAXOBSTRUCTIONLFRATIO      1.0f
#define EAXBUFFER_DEFAULTOBSTRUCTIONLFRATIO  0.0f

#define EAXBUFFER_MINOCCLUSION               (-10000)
#define EAXBUFFER_MAXOCCLUSION               0
#define EAXBUFFER_DEFAULTOCCLUSION           0

#define EAXBUFFER_MINOCCLUSIONLFRATIO        0.0f
#define EAXBUFFER_MAXOCCLUSIONLFRATIO        1.0f
#define EAXBUFFER_DEFAULTOCCLUSIONLFRATIO    0.25f

#define EAXBUFFER_MINOCCLUSIONROOMRATIO      0.0f
#define EAXBUFFER_MAXOCCLUSIONROOMRATIO      10.0f
#define EAXBUFFER_DEFAULTOCCLUSIONROOMRATIO  0.5f

#define EAXBUFFER_MINOUTSIDEVOLUMEHF         (-10000)
#define EAXBUFFER_MAXOUTSIDEVOLUMEHF         0
#define EAXBUFFER_DEFAULTOUTSIDEVOLUMEHF     0

#define EAXBUFFER_MINAIRABSORPTIONFACTOR     0.0f
#define EAXBUFFER_MAXAIRABSORPTIONFACTOR     10.0f
#define EAXBUFFER_DEFAULTAIRABSORPTIONFACTOR 1.0f

#define EAXBUFFER_DEFAULTFLAGS               (EAXBUFFERFLAGS_DIRECTHFAUTO | \
                                              EAXBUFFERFLAGS_ROOMAUTO |     \
                                              EAXBUFFERFLAGS_ROOMHFAUTO)

// Material transmission presets
// 3 values in this order:
//     1: occlusion (or obstruction)
//     2: occlusion LF Ratio (or obstruction LF Ratio)
//     3: occlusion Room Ratio

// Single window material preset
#define EAX_MATERIAL_SINGLEWINDOW          (-2800)
#define EAX_MATERIAL_SINGLEWINDOWLF        0.71f
#define EAX_MATERIAL_SINGLEWINDOWROOMRATIO 0.43f

// Double window material preset
#define EAX_MATERIAL_DOUBLEWINDOW          (-5000)
#define EAX_MATERIAL_DOUBLEWINDOWHF        0.40f
#define EAX_MATERIAL_DOUBLEWINDOWROOMRATIO 0.24f

// Thin door material preset
#define EAX_MATERIAL_THINDOOR              (-1800)
#define EAX_MATERIAL_THINDOORLF            0.66f
#define EAX_MATERIAL_THINDOORROOMRATIO     0.66f

// Thick door material preset
#define EAX_MATERIAL_THICKDOOR             (-4400)
#define EAX_MATERIAL_THICKDOORLF           0.64f
#define EAX_MATERIAL_THICKDOORROOMRTATION  0.27f

// Wood wall material preset
#define EAX_MATERIAL_WOODWALL              (-4000)
#define EAX_MATERIAL_WOODWALLLF            0.50f
#define EAX_MATERIAL_WOODWALLROOMRATIO     0.30f

// Brick wall material preset
#define EAX_MATERIAL_BRICKWALL             (-5000)
#define EAX_MATERIAL_BRICKWALLLF           0.60f
#define EAX_MATERIAL_BRICKWALLROOMRATIO    0.24f

// Stone wall material preset
#define EAX_MATERIAL_STONEWALL             (-6000)
#define EAX_MATERIAL_STONEWALLLF           0.68f
#define EAX_MATERIAL_STONEWALLROOMRATIO    0.20f

// Curtain material preset
#define EAX_MATERIAL_CURTAIN               (-1200)
#define EAX_MATERIAL_CURTAINLF             0.15f
#define EAX_MATERIAL_CURTAINROOMRATIO      1.00f


#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
