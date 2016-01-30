#include "stdafx.h"
#include "PerformanceAlert.hpp"
#include "xrEngine/GameFont.h"

void PerformanceAlert::Print(IGameFont &font, const char *format, ...)
{
    u32 refColor = font.GetColor();
    Fvector2 refPos = font.GetPosition();
    float refHeight = font.GetHeight();
    font.SetColor(alertColor);
    font.OutSet(alertPos.x, alertPos.y);
    font.SetHeight(fontBaseSize*2);
    va_list args;
    va_start(args, format);
    font.OutNextVA(format, args);
    va_end(args);
    alertPos = font.GetPosition();
    font.SetColor(refColor);
    font.OutSet(refPos.x, refPos.y);
    font.SetHeight(refHeight);
}
