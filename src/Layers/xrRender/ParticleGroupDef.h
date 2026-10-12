#pragma once

namespace xray::render::RENDER_NAMESPACE::PS
{
class ECORE_API CPGDef
{
public:
    shared_str m_Name;
    Flags32 m_Flags;
    float m_fTimeLimit;
    struct SEffect
    {
        enum
        {
            flDefferedStop = (1 << 0),
            flOnPlayChild = (1 << 1),
            flEnabled = (1 << 2),
            flOnPlayChildRewind = (1 << 4),
            flOnBirthChild = (1 << 5),
            flOnDeadChild = (1 << 6),
        };
        Flags32 m_Flags;
        shared_str m_EffectName;
        shared_str m_OnPlayChildName;
        shared_str m_OnBirthChildName;
        shared_str m_OnDeadChildName;
        float m_Time0;
        float m_Time1;
        SEffect()
        {
            m_Flags.zero(); /*set(flEnabled)*/
            m_Time0 = 0;
            m_Time1 = 0;
        }
#ifdef _EDITOR
        BOOL Equal(const SEffect&);
#endif
    };

    using EffectVec = xr_vector<SEffect*>;
    EffectVec m_Effects;
#ifdef _EDITOR
    // change Equal if variables changed
    void __stdcall OnEffectsEditClick(ButtonValue* sender, bool& bDataModified, bool& bSafe);
    void __stdcall OnEffectTypeChange(PropValue* sender);
    void __stdcall OnEffectEditClick(ButtonValue* sender, bool& bDataModified, bool& bSafe);
    void __stdcall OnControlClick(ButtonValue* sender, bool& bDataModified, bool& bSafe);
    void __stdcall OnParamsChange(PropValue* sender);
    void FillProp(LPCSTR pref, ::PropItemVec& items, ::ListItem* owner);
    BOOL Equal(const CPGDef* pe);
    bool Validate(bool bMsg);
#endif
public:
    CPGDef();
    ~CPGDef();
    void SetName(LPCSTR name);

    void Save(IWriter& F);
    BOOL Load(IReader& F);

    void Save2(CInifile& ini);
    BOOL Load2(CInifile& ini);

#ifdef _EDITOR
    void Clone(CPGDef* source);
#endif
};

} // namespace xray::render::RENDER_NAMESPACE::PS
