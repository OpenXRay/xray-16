#include "pch.hpp"
#include "ELog.h"
#include "Props/WindowLog.h"

gcroot<EditorLog^> ELog;

using namespace XRay::ECore::Props;

XRECORE_API void ELogCallback(void* context, pcstr message)
{
    if (0 == message[0])
        return;
    bool isDialog = ('#' == message[0]) || ((0 != message[1]) && ('#' == message[1]));
    MessageType type = ('!' == message[0]) || ((0 != message[1]) && ('!' == message[1])) ? MessageType::Error : MessageType::Information;
    if (('!' == message[0]) || ('#' == message[0]))
        message++;
    if (('!' == message[0]) || ('#' == message[0]))
        message++;

    auto windowLog = safe_cast<WindowLog^>(Form::FromHandle(IntPtr(context)));

    if (windowLog)
    {
        if (isDialog)
            windowLog->AddDialogMessage(type, message);
        else
            windowLog->AddMessage(type, message);
    }
}