#include "stdafx.h"
#include "r__buffer_pool.h"
#include "ModelPool.h"
#include "PSLibrary.h"

namespace xray::render::fg
{
R_buffer_pool BufferPool;
CModelPool* g_pModelPool = nullptr;
CDetailManager* g_pDetailManager = nullptr;
CWallmarksEngine* g_pWallmarksEngine = nullptr;
CPSLibrary* g_pPSLibrary = nullptr;
}
