#pragma once

//. #define MOD_COMPRESS

#include "xrCore/xrCore.h"

#include <mmsystem.h>
#include "lzo\lzo1x.h"

#pragma warning(disable : 4995)
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <sys\stat.h>
#pragma warning(default : 4995)

#pragma comment(lib, "xrCore.lib")
#pragma comment(lib, "winmm")