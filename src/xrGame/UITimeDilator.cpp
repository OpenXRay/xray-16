#include "UITimeDilator.h"
#include "StdAfx.h"

UITimeDilator* time_dilator;

UITimeDilator* TimeDilator()
{
    if (!time_dilator)
    {
        time_dilator = xr_new<UITimeDilator>();
    }

    return time_dilator;
}

void CloseTimeDilator()
{
    if (time_dilator)
    {
        xr_delete(time_dilator);
    }
}

void UITimeDilator::SetUiTimeFactor(float timeFactor)
{
    uiTimeFactor = timeFactor;
    startTimeDilation();
};

float UITimeDilator::GetUiTimeFactor() { return uiTimeFactor; };

void UITimeDilator::SetModeEnability(UIMode mode, bool status)
{
    enabledModes.set(mode, status);

    if (status)
    {
        startTimeDilation();
    }
    else if (!status && mode == currMode)
    {
        stopTimeDilation();
    }
}

bool UITimeDilator::GetModeEnability(UIMode mode) { return enabledModes.is(mode); }

void UITimeDilator::SetCurrentMode(UIMode mode)
{
    currMode = mode;
    if (mode != None)
    {
        startTimeDilation();
    }
    else
    {
        stopTimeDilation();
    }
}

void UITimeDilator::startTimeDilation()
{
    if (enabledModes.is(currMode) && currMode != None)
        Device.time_factor(uiTimeFactor);
}

void UITimeDilator::stopTimeDilation()
{
    Device.time_factor(1.0);
}
