#pragma once

class UIStyle
{
public:
    u32 UIStyleID = 0;
    xr_vector<xr_token> UIStyleToken;

    void FillUIStyleToken();
    void SetupUIStyle();
    void CleanupUIStyleToken();

private:
    bool defaultUIStyle = true;
};

extern UIStyle* UIStyleManager;
