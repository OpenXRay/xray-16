#pragma once

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrCore/_std_extensions.h"

#if defined(WINDOWS)
#pragma warning(push)
#pragma warning(disable : 4995)
#include <DPlay/dplay8.h>
#pragma warning(pop)
#endif

#include "NET_Shared.h"
