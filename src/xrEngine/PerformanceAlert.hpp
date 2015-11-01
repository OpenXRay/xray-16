#pragma once
#include "xrEngine/Engine.h"
#include "xrCore/xrCore.h"
#include "xrEngine/IPerformanceAlert.hpp"

class ENGINE_API PerformanceAlert : public IPerformanceAlert
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
    
    virtual void Reset() override { alertPos = initialAlertPos; }
    virtual void Print(class IGameFont &font, const char *format, ...) override;
};
