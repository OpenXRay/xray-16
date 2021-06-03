#pragma once
#define ENGINE_API
#define NO_XRC_STATS

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrCore/_std_extensions.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include "d3dx9.h"
#pragma warning(pop)

// Warnings
#pragma warning(disable : 4786) // too long names
#pragma warning(disable : 4503) // decorated name length exceeded
#pragma warning(disable : 4251) // object needs DLL interface
