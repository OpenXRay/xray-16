#pragma once

//. #define MOD_COMPRESS

#include "Common/Platform.hpp"
#include "xrCore/xrCore.h"

#include "lzo/lzo1x.h"
#include <mmsystem.h>

#pragma warning(push)
#pragma warning(disable : 4995)
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <malloc.h>
#pragma warning(pop)

#pragma comment(lib, "xrCore.lib")
#pragma comment(lib, "winmm")
