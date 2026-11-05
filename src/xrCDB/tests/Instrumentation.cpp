#include "xrCDB/stdafx.h"
#include "Instrumentation.h"

namespace CDB
{
static std::atomic<TestObserver> observer{};

void SetTestObserver(TestObserver value)
{
    observer.store(value);
}

void NotifyTestObserver(TestEvent event, const MODEL* model)
{
    if (auto callback = observer.load())
        callback(event, model);
}
}
