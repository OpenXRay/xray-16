// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef stdafxH
#define stdafxH
#pragma once

#include "xrCore/xrCore.h"

// mmsystem.h
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
#include <mmsystem.h>

// mmreg.h
#define NOMMIDS
#define NONEWRIFF
#define NOJPEGDIB
#define NONEWIC
#define NOBITMAP
#include <mmreg.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "xrCDB/xrCDB.h"
#include "Sound.h"

#undef ENGINE_API

#include "xrCore/xr_resource.h"

#ifdef _EDITOR
#include "utils/ETools/ETools.h"
#endif
// TODO: reference additional headers your program requires here
#endif
