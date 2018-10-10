#pragma once

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrCore/_std_extensions.h"

#if defined(WINDOWS)
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
#endif

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "xrCDB/xrCDB.h"
#include "Sound.h"

#include "xrCore/xr_resource.h"

#ifdef _EDITOR
#include "utils/ETools/ETools.h"
#endif
