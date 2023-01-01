#pragma once

class XRUICORE_API UIStyleManager : Noncopyable
{
    static constexpr u32 DEFAULT_STYLE = 0;

public:
    UIStyleManager();
    ~UIStyleManager();

    void SetupStyle(u32 styleID);
    void Reset();

    [[nodiscard]]
    auto GetCurrentStyleId() const
    {
        return m_style_id;
    }

    [[nodiscard]]
    bool DefaultStyleIsSet() const
    {
        return m_style_id == DEFAULT_STYLE;
    }

    [[nodiscard]]
    auto& GetToken() const
    {
        return m_token;
    }

private:
    xr_vector<xr_token> m_token;
    u32 m_style_id{ DEFAULT_STYLE };
};

XRUICORE_API extern UIStyleManager* UIStyles();
XRUICORE_API extern void CloseUIStyles();
