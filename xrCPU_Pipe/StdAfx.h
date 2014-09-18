#pragma once

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif // _WIN32_WINNT

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>

#pragma warning(disable:4995)
	#include <intrin.h>
#pragma warning(default:4995)

#define ENGINE_API
#define ECORE_API

#ifdef _EDITOR
#	include "skeletonX.h"
#	include "skeletoncustom.h"
#else // _EDITOR
	#include "../xrCore/xrCore.h"
	#include "../Layers/xrRender/SkeletonXVertRender.h"
	#include "../xrEngine/bone.h"
	#define RENDER 1
	#include "../xrEngine/Render.h"
	#include "../xrEngine/Device.h"
	#include "../Layers/xrRender/light.h"
#endif // _EDITOR

#include "xrCPU_Pipe.h"
#include "ttapi.h"