#pragma once

#include "Common/Config.hpp"
#include "Common/Platform.hpp"
#include "Common/CommonImportExport.inl"
#include "Common/FSMacros.hpp"
#include "Include/xrAPI/xrAPI.h"

#if __has_include(<SDL.h>)
#include <SDL.h>
#endif

#if __has_include(<SDL_syswm.h>)
#include <SDL_syswm.h>
#endif
