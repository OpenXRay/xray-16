#pragma once
#include "xrEngine/Engine.h"
#include "xrCore/xrCore.h"

class ENGINE_API PerformanceAlert
{
private:
    u32 alertColor;
    float fontBaseSize;
    Fvector2 initialAlertPos, alertPos;

public:
    PerformanceAlert(float fontBaseSize, const Fvector2 &alertPos)
    {
        alertColor = color_rgba(255, 16, 16, 255);
        this->fontBaseSize = fontBaseSize;
        this->initialAlertPos = alertPos;
        this->alertPos = alertPos;
        Reset();
    }
    
    void Reset() { alertPos = initialAlertPos; }
    void Print(class CGameFont &font, const char *format, ...);
};
