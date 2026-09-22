#pragma once
#include "xrUICore/Windows/UIFrameLineWnd.h"

class CUIStatic;
class CUIFrameLineWnd;
class CUIDetectorWave;
class CSimpleDetector;
class CAdvancedDetector;
class CEliteDetector;
class CUIXml;
class CLAItem;
class CBoneInstance;

class XR_NOVTABLE CUIArtefactDetectorBase
{
public:
    virtual ~CUIArtefactDetectorBase() = 0;
    virtual void update() {}
};

inline CUIArtefactDetectorBase::~CUIArtefactDetectorBase() = default;

class CUIDetectorWave final : public CUIFrameLineWnd
{
    typedef CUIFrameLineWnd inherited;

protected:
    float m_curr_v{};
    float m_step{};

public:
    CUIDetectorWave() : CUIFrameLineWnd(CUIDetectorWave::GetDebugType()) {}

    void InitFromXML(CUIXml& xml, LPCSTR path);
    void SetVelocity(float v);
    void Update() override;

    pcstr GetDebugType() override { return "CUIDetectorWave"; }
};

class CUIArtefactDetectorSimple final : public CUIArtefactDetectorBase
{
    typedef CUIArtefactDetectorBase inherited;

    CSimpleDetector* m_parent;
    u16 m_flash_bone{ BI_NONE };
    u16 m_on_off_bone{ BI_NONE };
    u32 m_turn_off_flash_time;

    ref_light m_flash_light;
    ref_light m_on_off_light;
    CLAItem* m_pOnOfLAnim{};
    CLAItem* m_pFlashLAnim{};
    void setup_internals();

public:
    CUIArtefactDetectorSimple(CSimpleDetector* parent);
    ~CUIArtefactDetectorSimple() override;
    void update() override;
    void Flash(bool bOn, float fRelPower);
};

class CUIArtefactDetectorAdv final : public CUIArtefactDetectorBase
{
    typedef CUIArtefactDetectorBase inherited;

    CAdvancedDetector* m_parent{};
    Fvector m_target_dir{};
    float m_cur_y_rot{};
    float m_curr_ang_speed{};
    u16 m_bid{ BI_NONE };

public:
    CUIArtefactDetectorAdv(CAdvancedDetector* parent) : m_parent(parent) {}

    void update() override;

    void SetValue(const Fvector& direction)
    {
        m_target_dir = direction;
    }

    float CurrentYRotation() const;
    static void BoneCallback(CBoneInstance* B);
    void ResetBoneCallbacks();
    void SetBoneCallbacks();
};

class CUIArtefactDetectorElite final : public CUIArtefactDetectorBase, public CUIWindow
{
    typedef CUIArtefactDetectorBase inherited;

    CUIWindow* m_wrk_area{};

    xr_map<shared_str, CUIStatic*> m_palette;

    struct SDrawOneItem
    {
        SDrawOneItem(CUIStatic* s, const Fvector& p) : pStatic(s), pos(p) {}
        CUIStatic* pStatic;
        Fvector pos;
    };
    xr_vector<SDrawOneItem> m_items_to_draw;
    CEliteDetector* m_parent{};
    Fmatrix m_map_attach_offset{};

    void GetUILocatorMatrix(Fmatrix& m) const;

public:
    CUIArtefactDetectorElite(CEliteDetector* parent);

    void update() override;
    void Draw() override;

    void Clear();
    void RegisterItemToDraw(const Fvector& p, const shared_str& palette_idx);

    pcstr GetDebugType() override { return "CUIArtefactDetectorElite"; }
};
