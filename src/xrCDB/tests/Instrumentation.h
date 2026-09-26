#pragma once
#include "Common/Common.hpp"
#include "xrCore/xrCore.h"

#include "xrCDB/xrCDB.h"

namespace CDB
{
enum class TestEvent
{
    BuildStarted,
    BuildReady,
    FrustumBounds,
    FrustumTriangle
};
using TestObserver = void (*)(TestEvent, const MODEL*);
XRCDB_API void SetTestObserver(TestObserver observer);
void NotifyTestObserver(TestEvent event, const MODEL* model);
}
