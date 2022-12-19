#pragma once
class UITimeDilator
{
public:

    enum UIMode
    {
        None = 0,
        Inventory = 1 << 0,
        Pda = 1 << 1
    };

    void SetUiTimeFactor(float timeFactor);
    float GetUiTimeFactor();

    void SetModeEnability(UIMode mode, bool status);
    bool GetModeEnability(UIMode mode);

    bool StartTimeDilation(UIMode mode);
    void StopTimeDilation();

private:
    float uiTimeFactor = 1.0;
    Flags32 enabledModes;
    UIMode currMode;

    bool dilateTime();
    void resetTime();
};  
