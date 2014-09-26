/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include <windows.h>
#include <dsound.h>
#include "gt2aMain.h"
#include "gt2aSound.h"
#include "gt2aMath.h"
#include "../../darray.h"

#define MAX_DUPLICATES   8

static GT2Bool playSounds;
static LPDIRECTSOUND DirectSound;
static DArray SoundList;

typedef struct SoundEffect
{
	LPDIRECTSOUNDBUFFER SoundBuffers[MAX_DUPLICATES];
	int numDuplicates;
} SoundEffect;

// The order of this list must match up with the order
// of the SOUND_* enums in gt2aSound.h
//////////////////////////////////////////////////////
const char * soundFiles[] =
{
	"sounds\\explosion.wav",
	"sounds\\mine.wav",
	"sounds\\die.wav",
	"sounds\\rocket.wav",
	"sounds\\pickup.wav",
	NULL
};

// Adapted from a Microsoft DirectSound sample.
///////////////////////////////////////////////
static HRESULT ReadMMIO
(
	HMMIO hmmioIn,
	MMCKINFO * pckInRIFF,
	WAVEFORMATEX ** ppwfxInfo
)
{
    MMCKINFO        ckIn;           // chunk info. for general use.
    PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       
	
    *ppwfxInfo = NULL;
	
    if( ( 0 != mmioDescend( hmmioIn, pckInRIFF, NULL, 0 ) ) )
        return E_FAIL;
	
    if( (pckInRIFF->ckid != FOURCC_RIFF) ||
        (pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E') ) )
        return E_FAIL;
	
    // Search the input file for for the 'fmt ' chunk.
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if( 0 != mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK) )
        return E_FAIL;
	
    // Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    // if there are extra parameters at the end, we'll ignore them
	if( ckIn.cksize < (LONG) sizeof(PCMWAVEFORMAT) )
		return E_FAIL;
	
    // Read the 'fmt ' chunk into <pcmWaveFormat>.
    if( mmioRead( hmmioIn, (HPSTR) &pcmWaveFormat, 
		sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat) )
        return E_FAIL;
	
    // Allocate the waveformatex, but if its not pcm format, read the next
    // word, and thats how many extra bytes to allocate.
    if( pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM )
    {
        if( NULL == ( *ppwfxInfo = (WAVEFORMATEX *)malloc(sizeof(WAVEFORMATEX))))
            return E_FAIL;
		
        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat) );
        (*ppwfxInfo)->cbSize = 0;
    }
    else
    {
        // Read in length of extra bytes.
        WORD cbExtraBytes = 0L;
        if( mmioRead( hmmioIn, (CHAR*)&cbExtraBytes, sizeof(WORD)) != sizeof(WORD) )
            return E_FAIL;
		
        *ppwfxInfo = (WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX) + cbExtraBytes);
        if( NULL == *ppwfxInfo )
            return E_FAIL;
		
        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat) );
        (*ppwfxInfo)->cbSize = cbExtraBytes;
		
        // Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
        if( mmioRead( hmmioIn, (CHAR*)(((BYTE*)&((*ppwfxInfo)->cbSize))+sizeof(WORD)),
			cbExtraBytes ) != cbExtraBytes )
        {
            free(*ppwfxInfo);
            *ppwfxInfo = NULL;
            return E_FAIL;
        }
    }
	
    // Ascend the input file out of the 'fmt ' chunk.
    if( 0 != mmioAscend( hmmioIn, &ckIn, 0 ) )
    {
        free(*ppwfxInfo);
        *ppwfxInfo = NULL;
        return E_FAIL;
    }
	
    return S_OK;
}

// Adapted from a Microsoft DirectSound sample.
///////////////////////////////////////////////
static HRESULT WaveOpenFile
(
	CHAR * strFileName,
	HMMIO * phmmioIn,
	WAVEFORMATEX ** ppwfxInfo,
	MMCKINFO * pckInRIFF
)
{
    HRESULT hr;
    HMMIO   hmmioIn = NULL;
    
    if( NULL == ( hmmioIn = mmioOpen( strFileName, NULL, MMIO_ALLOCBUF|MMIO_READ ) ) )
        return E_FAIL;
	
    if( FAILED( hr = ReadMMIO( hmmioIn, pckInRIFF, ppwfxInfo ) ) )
    {
        mmioClose( hmmioIn, 0 );
        return hr;
    }
	
    *phmmioIn = hmmioIn;
	
    return S_OK;
}

// Adapted from a Microsoft DirectSound sample.
///////////////////////////////////////////////
static HRESULT WaveStartDataRead
(
	HMMIO * phmmioIn,
	MMCKINFO * pckIn,
	MMCKINFO * pckInRIFF
)
{
    // Seek to the data
    if( -1 == mmioSeek( *phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC),
		SEEK_SET ) )
        return E_FAIL;
	
    // Search the input file for for the 'data' chunk.
    pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
    if( 0 != mmioDescend( *phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK ) )
        return E_FAIL;
	
    return S_OK;
}

// Adapted from a Microsoft DirectSound sample.
///////////////////////////////////////////////
static HRESULT WaveReadFile
(
	HMMIO hmmioIn,
	UINT cbRead,
	BYTE * pbDest,
	MMCKINFO * pckIn,
	UINT * cbActualRead
)
{
    MMIOINFO mmioinfoIn;         // current status of <hmmioIn>
	UINT cbDataIn;
	DWORD cT;
	
    *cbActualRead = 0;
	
    if( 0 != mmioGetInfo( hmmioIn, &mmioinfoIn, 0 ) )
        return E_FAIL;
	
    cbDataIn = cbRead;
    if( cbDataIn > pckIn->cksize ) 
        cbDataIn = pckIn->cksize;       
	
    pckIn->cksize -= cbDataIn;
    
    for(cT = 0; cT < cbDataIn; cT++ )
    {
        // Copy the bytes from the io to the buffer.
        if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
        {
            if( 0 != mmioAdvance( hmmioIn, &mmioinfoIn, MMIO_READ ) )
                return E_FAIL;
			
            if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
                return E_FAIL;
        }
		
        // Actual copy.
        *((BYTE*)pbDest+cT) = *((BYTE*)mmioinfoIn.pchNext);
        mmioinfoIn.pchNext++;
    }
	
    if( 0 != mmioSetInfo( hmmioIn, &mmioinfoIn, 0 ) )
        return E_FAIL;
	
    *cbActualRead = cbDataIn;
    return S_OK;
}

// If restoreBuffer is not NULL, restore it!
////////////////////////////////////////////
static GT2Bool LoadSound
(
	int sound,
	const char * filename,
	LPDIRECTSOUNDBUFFER restoreBuffer
)
{
	int rcode;
	HRESULT result;
	LPWAVEFORMATEX waveFormat;
	HMMIO IOHandle;
	MMCKINFO dataChunkInfo;
	MMCKINFO parentChunkInfo;
	DSBUFFERDESC bufferDescriptor;
	SoundEffect soundEffect;
	LPVOID lockedBuffer;
	DWORD lockedBufferLen;
	UINT numBytes;
	LPDIRECTSOUNDBUFFER soundBuffer;

	// Open the file.
	/////////////////
	rcode = WaveOpenFile((char *)filename, &IOHandle, &waveFormat, &parentChunkInfo);
	if(rcode)
		return GT2False;

	// Prepare to read the data chunk.
	//////////////////////////////////
	rcode = WaveStartDataRead(&IOHandle, &dataChunkInfo, &parentChunkInfo);
	if(rcode)
		return GT2False;

	// If we're not restoring, create the buffer.
	/////////////////////////////////////////////
	if(!restoreBuffer)
	{
		// Clear our sound object.
		//////////////////////////
		memset(&soundEffect, 0, sizeof(SoundEffect));
		soundEffect.numDuplicates = 1;

		// Describe the desired buffer.
		///////////////////////////////
		memset(&bufferDescriptor, 0, sizeof(DSBUFFERDESC));
		bufferDescriptor.dwSize = sizeof(DSBUFFERDESC);
		bufferDescriptor.dwFlags = DSBCAPS_STATIC;
		bufferDescriptor.dwBufferBytes = dataChunkInfo.cksize;
		bufferDescriptor.lpwfxFormat = waveFormat;

		// Create the buffer.
		/////////////////////
		result = IDirectSound_CreateSoundBuffer(
			DirectSound,
			&bufferDescriptor,
			&soundEffect.SoundBuffers[0],
			NULL);
		if(FAILED(result))
		{
			mmioClose(IOHandle, 0);
			return GT2False;
		}

		// Set the sound buffer pointer.
		////////////////////////////////
		soundBuffer = soundEffect.SoundBuffers[0];
	}
	else
	{
		// Set the sound buffer pointer.
		////////////////////////////////
		soundBuffer = restoreBuffer;
	}

	// Lock the buffer.
	///////////////////
	result = IDirectSoundBuffer_Lock(
		soundBuffer,
		0,
		0,
		&lockedBuffer,
		&lockedBufferLen,
		NULL,
		NULL,
		DSBLOCK_ENTIREBUFFER);
	if(FAILED(result))
	{
		mmioClose(IOHandle, 0);
		return GT2False;
	}

	// Read the data into the buffer.
	/////////////////////////////////
	rcode = WaveReadFile(
		IOHandle,
		lockedBufferLen,
		(BYTE *)lockedBuffer,
		&dataChunkInfo,
		&numBytes);
	if(rcode)
	{
		mmioClose(IOHandle, 0);
		return GT2False;
	}

	// Unlock the buffer.
	/////////////////////
	IDirectSoundBuffer_Unlock(
		soundBuffer,
		lockedBuffer,
		lockedBufferLen,
		NULL,
		0);

	// Close the file.
	//////////////////
	mmioClose(IOHandle, 0);

	// Add this sound to the list if its new.
	/////////////////////////////////////////
	if(!restoreBuffer)
		ArrayAppend(SoundList, &soundEffect);

	return GT2True;
}

GT2Bool InitializeSound
(
	void
)
{
	HRESULT result;
	HWND hWnd;
	int i;

	// Init the sound list.
	///////////////////////
	SoundList = ArrayNew(sizeof(SoundEffect), 0, NULL);
	if(!SoundList)
		return GT2False;

	// Init direct sound.
	/////////////////////
	result = DirectSoundCreate(NULL, &DirectSound, NULL);
	if(FAILED(result))
		return GT2False;

	// Get the window handle.
	/////////////////////////
	hWnd = GetForegroundWindow();
	if(!hWnd)
		hWnd = GetDesktopWindow();

	// Set the cooperative level.
	/////////////////////////////
	result = IDirectSound_SetCooperativeLevel(DirectSound, hWnd, DSSCL_PRIORITY);
	if(FAILED(result))
		return GT2False;

	// Load the sounds.
	///////////////////
	for(i = 0 ; i < NUM_SOUNDS ; i++)
	{
		assert(soundFiles[i]);
		if(!soundFiles[i])
			return GT2False;
		LoadSound(i, soundFiles[i], NULL);
	}
	assert(!soundFiles[i]);

	// Turn on sound.
	/////////////////
	playSounds = GT2True;

	return GT2True;
}

void CleanupSound
(
	void
)
{
	// Cleanup the sound list.
	//////////////////////////
	ArrayFree(SoundList);

	// Cleanup DirectSound.
	///////////////////////
	if(DirectSound)
	{
		IDirectSound_Release(DirectSound);
		DirectSound = NULL;
	}

	// COM crap.
	////////////
	CoUninitialize();
}

void ToggleSound
(
	void
)
{
	playSounds = !playSounds;
}

void PlaySoundEffect
(
	int sound
)
{
	SoundEffect * soundEffect;
	HRESULT result;
	DWORD status;
	int i;

	// Make sure we're playing sounds.
	//////////////////////////////////
	if(!playSounds)
		return;

	// Get the correct sound.
	/////////////////////////
	soundEffect = (SoundEffect *)ArrayNth(SoundList, sound);
	assert(soundEffect);
	if(!soundEffect)
		return;

	// Find a free buffer.
	//////////////////////
	for(i = 0 ; i < soundEffect->numDuplicates ; i++)
	{
		// Get the current status.
		//////////////////////////
		result = IDirectSoundBuffer_GetStatus(soundEffect->SoundBuffers[i], &status);
		if(FAILED(result))
			return;

		// Did it lose its buffer?
		//////////////////////////
		if(status & DSBSTATUS_BUFFERLOST)
		{
			// Restore the buffer.
			//////////////////////
			result = IDirectSoundBuffer_Restore(soundEffect->SoundBuffers[i]);
			if(FAILED(result))
				continue;

			// Fill the sound data back in.
			///////////////////////////////
			if(!LoadSound(sound, soundFiles[sound], soundEffect->SoundBuffers[i]))
				continue;
		}

		// Is it free?
		//////////////
		if(!(status & DSBSTATUS_PLAYING))
			break;
	}

	// Did we not find anything?
	////////////////////////////
	if(i == soundEffect->numDuplicates)
	{
		// Are we at max duplicates?
		////////////////////////////
		if(i == MAX_DUPLICATES)
		{
			// Just use a random buffer.
			////////////////////////////
			i = RandomInt(0, MAX_DUPLICATES - 1);
		}
		else
		{
			// Make a new duplicate.
			////////////////////////
			result = IDirectSound_DuplicateSoundBuffer(
				DirectSound,
				soundEffect->SoundBuffers[0],
				&soundEffect->SoundBuffers[i]);
			if(FAILED(result))
				return;

			// One more duplicate.
			//////////////////////
			soundEffect->numDuplicates++;
		}
	}

	// Make sure the buffer is rewound.
	///////////////////////////////////
	IDirectSoundBuffer_SetCurrentPosition(soundEffect->SoundBuffers[i], 0);

	// Play it.
	///////////
	result = IDirectSoundBuffer_Play(soundEffect->SoundBuffers[i], 0, 0, 0);

	// Check for a lost buffer.
	///////////////////////////
	if(result == DSERR_BUFFERLOST)
	{
		// Restore the buffer.
		//////////////////////
		result = IDirectSoundBuffer_Restore(soundEffect->SoundBuffers[i]);
		if(FAILED(result))
			return;

		// Fill the sound data back in.
		///////////////////////////////
		if(!LoadSound(sound, soundFiles[sound], soundEffect->SoundBuffers[i]))
			return;

		// Try playing again.
		/////////////////////
		IDirectSoundBuffer_Play(soundEffect->SoundBuffers[i], 0, 0, 0);
	}
}