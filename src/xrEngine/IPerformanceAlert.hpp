#pragma once

class IGameFont;

class IPerformanceAlert
{
public:
    virtual void Reset() = 0;
    virtual void Print(IGameFont &font, const char *format, ...) = 0;
};
