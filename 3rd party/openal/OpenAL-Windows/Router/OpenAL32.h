/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */




#ifndef _OPENAL32_H_
#define _OPENAL32_H_

#ifdef __cplusplus
extern "C" {
#endif

#define AL_BUILD_LIBRARY

#include "openal\al.h"

// ALAPI
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_ENABLE)(ALenum capability);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_DISABLE)(ALenum capability);
typedef ALAPI ALboolean      (ALAPIENTRY *ALAPI_IS_ENABLED)(ALenum capability);

typedef ALAPI const ALchar*  (ALAPIENTRY *ALAPI_GET_STRING)(ALenum param);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_BOOLEANV)(ALenum param, ALboolean* data);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_INTEGERV)(ALenum param, ALint* data);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_FLOATV)(ALenum param, ALfloat* data);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_DOUBLEV)(ALenum param, ALdouble* data);
typedef ALAPI ALboolean      (ALAPIENTRY *ALAPI_GET_BOOLEAN)(ALenum param);
typedef ALAPI ALint          (ALAPIENTRY *ALAPI_GET_INTEGER)(ALenum param);
typedef ALAPI ALfloat        (ALAPIENTRY *ALAPI_GET_FLOAT)(ALenum param);
typedef ALAPI ALdouble       (ALAPIENTRY *ALAPI_GET_DOUBLE)(ALenum param);

typedef ALAPI ALenum         (ALAPIENTRY *ALAPI_GET_ERROR)(ALvoid);

typedef ALAPI ALboolean      (ALAPIENTRY *ALAPI_IS_EXTENSION_PRESENT)(const ALchar* ename);
typedef ALAPI ALvoid*        (ALAPIENTRY *ALAPI_GET_PROC_ADDRESS)(const ALchar* fname);
typedef ALAPI ALenum         (ALAPIENTRY *ALAPI_GET_ENUM_VALUE)(const ALchar* ename);

typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_LISTENERF)(ALenum param, ALfloat value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_LISTENER3F)(ALenum param, ALfloat v1, ALfloat v2, ALfloat v3);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_LISTENERFV)(ALenum param, const ALfloat* values);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_LISTENERI)(ALenum param, ALint value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_LISTENER3I)( ALenum param, ALint value1, ALint value2, ALint value3 );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_LISTENERIV)( ALenum param, const ALint* values );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_LISTENERF)(ALenum param, ALfloat* value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_LISTENER3F)(ALenum param, ALfloat* v1, ALfloat* v2, ALfloat* v3);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_LISTENERFV)(ALenum param, ALfloat* values);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_LISTENERI)(ALenum param, ALint* value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_LISTENER3I)( ALenum param, ALint *value1, ALint *value2, ALint *value3 );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_LISTENERIV)( ALenum param, ALint* values );

typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GEN_SOURCES)(ALsizei n, ALuint* sourceNames);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_DELETE_SOURCES)(ALsizei n, const ALuint* sourceNames);
typedef ALAPI ALboolean      (ALAPIENTRY *ALAPI_IS_SOURCE)(ALuint id);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCEF)(ALuint sourceName, ALenum param, ALfloat value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE3F)(ALuint sourceName, ALenum param, ALfloat v1, ALfloat v2, ALfloat v3);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCEFV)(ALuint sourceName, ALenum param, const ALfloat* values);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCEI)(ALuint sourceName, ALenum param, ALint value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE3I)( ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3 );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCEIV)( ALuint sid, ALenum param, const ALint* values );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_SOURCEF)(ALuint sourceName, ALenum param, ALfloat* value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_SOURCE3F)(ALuint sourceName, ALenum param, ALfloat* v1, ALfloat* v2, ALfloat* v3);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_SOURCEFV)(ALuint sourceName, ALenum param, const ALfloat* values);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_SOURCEI)(ALuint sourceName, ALenum param, ALint* value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_SOURCE3I)( ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_SOURCEIV)( ALuint sid, ALenum param, ALint* values );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_PLAYV)(ALsizei n, const ALuint* sources);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_STOPV)(ALsizei n, const ALuint* sources);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_REWINDV)(ALsizei n, const ALuint* sources);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_PAUSEV)(ALsizei n, const ALuint* sources);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_PLAY)(ALuint sourceName);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_STOP)(ALuint sourceName);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_REWIND)(ALuint sourceName);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_PAUSE)(ALuint sourceName);

typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_QUEUE_BUFFERS)(ALuint sourceName, ALsizei n, const ALuint* bufferNames);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SOURCE_UNQUEUE_BUFFERS)(ALuint sourceName, ALsizei n, ALuint* bufferNames);

typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GEN_BUFFERS)(ALsizei n, ALuint* bufferNames);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_DELETE_BUFFERS)(ALsizei n, const ALuint* bufferNames);
typedef ALAPI ALboolean      (ALAPIENTRY *ALAPI_IS_BUFFER)(ALuint bufferName);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_BUFFER_DATA)(ALuint bufferName, ALenum format, const ALvoid* data, ALsizei size, ALuint freq);

typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_BUFFERF)( ALuint bid, ALenum param, ALfloat value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_BUFFER3F)( ALuint bid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_BUFFERFV)( ALuint bid, ALenum param, const ALfloat* values );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_BUFFERI)( ALuint bid, ALenum param, ALint value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_BUFFER3I)( ALuint bid, ALenum param, ALint value1, ALint value2, ALint value3 );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_BUFFERIV)( ALuint bid, ALenum param, const ALint* values );

typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_BUFFERF)( ALuint bid, ALenum param, ALfloat* value );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_BUFFER3F)( ALuint bid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_BUFFERFV)( ALuint bid, ALenum param, ALfloat* values );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_BUFFERI)( ALuint bid, ALenum param, ALint* value );
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_BUFFER3I)( ALuint bid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_GET_BUFFERIV)( ALuint bid, ALenum param, ALint* values );

typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_DOPPLER_FACTOR)(ALfloat value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_DOPPLER_VELOCITY)(ALfloat value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_SPEED_OF_SOUND)(ALfloat value);
typedef ALAPI ALvoid         (ALAPIENTRY *ALAPI_DISTANCE_MODEL)(ALenum value);

typedef struct ALAPI_FXN_TABLE_STRUCT
{
    ALAPI_ENABLE                 alEnable;
    ALAPI_DISABLE                alDisable;
    ALAPI_IS_ENABLED             alIsEnabled;

	ALAPI_GET_STRING             alGetString;
	ALAPI_GET_BOOLEANV           alGetBooleanv;
    ALAPI_GET_INTEGERV           alGetIntegerv;
    ALAPI_GET_FLOATV             alGetFloatv;
    ALAPI_GET_DOUBLEV            alGetDoublev;
    ALAPI_GET_BOOLEAN            alGetBoolean;
    ALAPI_GET_INTEGER            alGetInteger;
    ALAPI_GET_FLOAT              alGetFloat;
    ALAPI_GET_DOUBLE             alGetDouble;    

    ALAPI_GET_ERROR              alGetError;

    ALAPI_IS_EXTENSION_PRESENT   alIsExtensionPresent;
    ALAPI_GET_PROC_ADDRESS       alGetProcAddress;
    ALAPI_GET_ENUM_VALUE         alGetEnumValue;

	ALAPI_LISTENERF              alListenerf;
    ALAPI_LISTENER3F             alListener3f;
    ALAPI_LISTENERFV             alListenerfv;
    ALAPI_LISTENERI              alListeneri;
	ALAPI_LISTENER3I			 alListener3i;
	ALAPI_LISTENERIV			 alListeneriv;
    
	ALAPI_GET_LISTENERF          alGetListenerf;
    ALAPI_GET_LISTENER3F         alGetListener3f;
    ALAPI_GET_LISTENERFV         alGetListenerfv;
    ALAPI_GET_LISTENERI          alGetListeneri;
	ALAPI_GET_LISTENER3I		 alGetListener3i;
	ALAPI_GET_LISTENERIV		 alGetListeneriv;

    ALAPI_GEN_SOURCES            alGenSources;
    ALAPI_DELETE_SOURCES         alDeleteSources;
    ALAPI_IS_SOURCE              alIsSource;
    ALAPI_SOURCEF                alSourcef;
    ALAPI_SOURCE3F               alSource3f;
    ALAPI_SOURCEFV               alSourcefv;
	ALAPI_SOURCEI                alSourcei;
	ALAPI_SOURCE3I				 alSource3i;
	ALAPI_SOURCEIV				 alSourceiv;
    ALAPI_GET_SOURCEF            alGetSourcef;
    ALAPI_GET_SOURCE3F           alGetSource3f;
    ALAPI_GET_SOURCEFV           alGetSourcefv;
	ALAPI_GET_SOURCEI            alGetSourcei;
	ALAPI_GET_SOURCE3I			 alGetSource3i;
	ALAPI_GET_SOURCEIV			 alGetSourceiv;
    ALAPI_SOURCE_PLAYV           alSourcePlayv;
    ALAPI_SOURCE_STOPV           alSourceStopv;
    ALAPI_SOURCE_REWINDV         alSourceRewindv;
	ALAPI_SOURCE_PAUSEV          alSourcePausev;
    ALAPI_SOURCE_PLAY            alSourcePlay;
    ALAPI_SOURCE_STOP            alSourceStop;
    ALAPI_SOURCE_REWIND          alSourceRewind;
	ALAPI_SOURCE_PAUSE           alSourcePause;

	ALAPI_SOURCE_QUEUE_BUFFERS   alSourceQueueBuffers;
    ALAPI_SOURCE_UNQUEUE_BUFFERS alSourceUnqueueBuffers;

    ALAPI_GEN_BUFFERS            alGenBuffers;
    ALAPI_DELETE_BUFFERS         alDeleteBuffers;
    ALAPI_IS_BUFFER              alIsBuffer;
    ALAPI_BUFFER_DATA            alBufferData;
	ALAPI_BUFFERF				 alBufferf;
	ALAPI_BUFFER3F				 alBuffer3f;
	ALAPI_BUFFERFV				 alBufferfv;
	ALAPI_BUFFERI				 alBufferi;
	ALAPI_BUFFER3I				 alBuffer3i;
	ALAPI_BUFFERIV				 alBufferiv;
    ALAPI_GET_BUFFERF            alGetBufferf;
    ALAPI_GET_BUFFER3F			 alGetBuffer3f;
	ALAPI_GET_BUFFERFV			 alGetBufferfv;
	ALAPI_GET_BUFFERI            alGetBufferi;
    ALAPI_GET_BUFFER3I			 alGetBuffer3i;
	ALAPI_GET_BUFFERIV			 alGetBufferiv;

    ALAPI_DOPPLER_FACTOR         alDopplerFactor;
    ALAPI_DOPPLER_VELOCITY       alDopplerVelocity;
	ALAPI_SPEED_OF_SOUND         alSpeedOfSound;
	ALAPI_DISTANCE_MODEL         alDistanceModel;

} ALAPI_FXN_TABLE;

#include "openal\alc.h"

// ALCAPI
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;

typedef ALCAPI ALCcontext* (ALCAPIENTRY *ALCAPI_CREATE_CONTEXT)(ALCdevice* device, const ALint* attrList);
typedef ALCAPI ALboolean   (ALCAPIENTRY *ALCAPI_MAKE_CONTEXT_CURRENT)(ALCcontext* context);
typedef ALCAPI ALvoid      (ALCAPIENTRY *ALCAPI_PROCESS_CONTEXT)(ALCcontext* context);
typedef ALCAPI ALCvoid     (ALCAPIENTRY *ALCAPI_SUSPEND_CONTEXT)(ALCcontext* context);
typedef ALCAPI ALvoid      (ALCAPIENTRY *ALCAPI_DESTROY_CONTEXT)(ALCcontext* context);
typedef ALCAPI ALCcontext* (ALCAPIENTRY *ALCAPI_GET_CURRENT_CONTEXT)(ALvoid);
typedef ALCAPI ALCdevice*  (ALCAPIENTRY *ALCAPI_GET_CONTEXTS_DEVICE)(ALCcontext* context);

typedef ALCAPI ALCdevice*  (ALCAPIENTRY *ALCAPI_OPEN_DEVICE)(const ALCchar* deviceName);
typedef ALCAPI ALCboolean  (ALCAPIENTRY *ALCAPI_CLOSE_DEVICE)(ALCdevice* device);

typedef ALCAPI ALenum      (ALCAPIENTRY *ALCAPI_GET_ERROR)(ALCdevice* device);

typedef ALCAPI ALboolean   (ALCAPIENTRY *ALCAPI_IS_EXTENSION_PRESENT)(ALCdevice* device, const ALCchar* eName);
typedef ALCAPI ALvoid*     (ALCAPIENTRY *ALCAPI_GET_PROC_ADDRESS)(ALCdevice* device, const ALCchar* fName);
typedef ALCAPI ALenum      (ALCAPIENTRY *ALCAPI_GET_ENUM_VALUE)(ALCdevice* device, const ALCchar* eName);

typedef ALCAPI const ALCchar*  (ALCAPIENTRY *ALCAPI_GET_STRING)(ALCdevice* device, ALenum param);
typedef ALCAPI ALvoid          (ALCAPIENTRY *ALCAPI_GET_INTEGERV)(ALCdevice* device, ALenum param, ALsizei size, ALint* data);

typedef ALCAPI ALCdevice * (ALCAPIENTRY *ALCAPI_CAPTURE_OPEN_DEVICE)( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
typedef ALCAPI ALCboolean  (ALCAPIENTRY *ALCAPI_CAPTURE_CLOSE_DEVICE)( ALCdevice *device );
typedef ALCAPI void        (ALCAPIENTRY *ALCAPI_CAPTURE_START)( ALCdevice *device );
typedef ALCAPI void        (ALCAPIENTRY *ALCAPI_CAPTURE_STOP)( ALCdevice *device );
typedef ALCAPI void        (ALCAPIENTRY *ALCAPI_CAPTURE_SAMPLES)( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );

typedef struct ALCAPI_FXN_TABLE_STRUCT
{
	ALCAPI_CREATE_CONTEXT       alcCreateContext;
    ALCAPI_MAKE_CONTEXT_CURRENT alcMakeContextCurrent;
    ALCAPI_PROCESS_CONTEXT      alcProcessContext;
	ALCAPI_SUSPEND_CONTEXT      alcSuspendContext;
    ALCAPI_DESTROY_CONTEXT      alcDestroyContext;
	ALCAPI_GET_CURRENT_CONTEXT  alcGetCurrentContext;
    ALCAPI_GET_CONTEXTS_DEVICE  alcGetContextsDevice;

	ALCAPI_OPEN_DEVICE          alcOpenDevice;
    ALCAPI_CLOSE_DEVICE         alcCloseDevice;

	ALCAPI_GET_ERROR            alcGetError;

    ALCAPI_IS_EXTENSION_PRESENT alcIsExtensionPresent;
    ALCAPI_GET_PROC_ADDRESS     alcGetProcAddress;
    ALCAPI_GET_ENUM_VALUE       alcGetEnumValue;

    ALCAPI_GET_STRING           alcGetString;
    ALCAPI_GET_INTEGERV         alcGetIntegerv;

	ALCAPI_CAPTURE_OPEN_DEVICE  alcCaptureOpenDevice;
	ALCAPI_CAPTURE_CLOSE_DEVICE alcCaptureCloseDevice;
	ALCAPI_CAPTURE_START        alcCaptureStart;
	ALCAPI_CAPTURE_STOP         alcCaptureStop;
	ALCAPI_CAPTURE_SAMPLES      alcCaptureSamples;

} ALCAPI_FXN_TABLE;

#include "windows.h"
#include "alList.h"


//*****************************************************************************
// Additional Defines
//*****************************************************************************

typedef struct ALCdevice_struct
{
    //
    // These variables must always be initialized.
    //
    ALenum                      LastError;
    ALint                       InUse;

    //
    // Support for 3rd party OpenAL implementations.
    //
    HINSTANCE                   Dll;
    ALCAPI_FXN_TABLE            AlcApi;

	// Capture Device
	ALCdevice*					CaptureDevice;

    struct ALCdevice_struct*     DllDevice;

} ALCdevice;

typedef struct ALCcontext_struct
{
    //
    // These variables are always initialized.
    //
    ALlistEntry                 ListEntry;
    ALboolean                   Suspended;
    ALenum                      LastError;
    ALCdevice*                   Device;
    ALAPI_FXN_TABLE             AlApi;
    CRITICAL_SECTION            Lock;
    struct ALCcontext_struct*    DllContext;

} ALCcontext;


extern ALlist* alContextList;
extern ALCcontext* alCurrentContext;

#ifdef __cplusplus
}
#endif

#endif

