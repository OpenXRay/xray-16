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

    void SetCurrentMode(UIMode mode);

private:
    float uiTimeFactor = 1.0;
    Flags32 enabledModes;
    UIMode currMode;

    void startTimeDilation();
    void stopTimeDilation();
};

extern UITimeDilator* TimeDilator();
extern void CloseTimeDilator();
