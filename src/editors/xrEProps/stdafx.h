#pragma once

#include <xrCore/xrCore.h>

#define smart_cast dynamic_cast

// refs
namespace CDB
{
class MODEL;
};

#include "xrServerEntities/xrEProps.h"

#include "FolderLib.h"

#include "Common/Platform.hpp"

#define ENGINE_API
#define DLL_API XR_IMPORT
#define ECORE_API XR_EXPORT

#include "Defines.h"

// libs
#pragma comment(lib, "xrSoundB.lib")
#pragma comment(lib, "xrCoreB.lib")
#pragma comment(lib, "EToolsB.lib")
