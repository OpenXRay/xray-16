#include "UITimeDilator.h"
#include "StdAfx.h"

void UITimeDilator::SetUiTimeFactor(float timeFactor)
{
    uiTimeFactor = timeFactor;
    dilateTime();
};

float UITimeDilator::GetUiTimeFactor() { return uiTimeFactor; };

void UITimeDilator::SetModeEnability(UIMode mode, bool status)
{
    enabledModes.set(mode, status);

    if (!status && mode == currMode)
    {
        resetTimeDilation();
    }
    else
    {
        dilateTime();
    }
}

bool UITimeDilator::GetModeEnability(UIMode mode) { return enabledModes.is(mode); }

bool UITimeDilator::StartTimeDilation(UIMode mode)
{
    currMode = mode;
    return dilateTime();
}

void UITimeDilator::StopTimeDilation()
{
    currMode = None;
    resetTimeDilation();
}

bool UITimeDilator::dilateTime()
{
    if (enabledModes.is(currMode) && currMode != None)
    {
        Device.time_factor(uiTimeFactor);
        return true;
    }

    return false;
}

void UITimeDilator::resetTimeDilation()
{
    if (enabledModes.is(currMode))
    {
        Device.time_factor(1.0);
    }
}
