#pragma once

class XRUICORE_API UIStyleManager : Noncopyable
{
    static constexpr u32 DEFAULT_STYLE_ID = 0;

public:
    UIStyleManager();
    ~UIStyleManager();

    void SetupStyle(u32 styleID);
    void Reset();

    bool SetStyle(pcstr name, bool reloadUI);

    [[nodiscard]]
    pcstr GetCurrentStyleName() const;

    [[nodiscard]]
    auto GetCurrentStyleId() const
    {
        return m_style_id;
    }

    [[nodiscard]]
    bool DefaultStyleIsSet() const
    {
        return m_style_id == DEFAULT_STYLE_ID;
    }

    [[nodiscard]]
    auto& GetToken() const
    {
        return m_token;
    }

private:
    xr_vector<xr_token> m_token;
    u32 m_style_id{ DEFAULT_STYLE_ID };
};

XRUICORE_API extern UIStyleManager* UIStyles;
