#ifndef ESceneCustomMToolsH
#define ESceneCustomMToolsH

#include "ESceneClassList.h"
// refs
struct SSceneSummary;
class TUI_CustomControl;
class ESceneCustomMTools;
class SceneBuilder;
struct SExportStreams;
struct mesh_build_data;
DEFINE_VECTOR(TUI_CustomControl*, ControlsVec, ControlsIt);

class ESceneToolBase
{
    ObjClassID FClassID;

protected:
    // controls
    ControlsVec m_Controls;
    int action;
    int sub_target;

public:
    enum
    {
        flEnable = (1 << 0),
        flReadonly = (1 << 1),
        flForceReadonly = (1 << 2),
        flChanged = (1 << 3),
        flVisible = (1 << 4),
    };

    Flags32 m_EditFlags;

    BOOL IsEnabled() { return m_EditFlags.is(flEnable); }
    BOOL IsEditable() { return !m_EditFlags.is_any(flReadonly | flForceReadonly); }
    BOOL IsReadonly() { return m_EditFlags.is(flReadonly); }
    BOOL IsForceReadonly() { return m_EditFlags.is(flForceReadonly); }
    BOOL IsChanged() { return m_EditFlags.is(flChanged); }
    void SetChanged(BOOL b) { m_EditFlags.set(flChanged, b); }
    BOOL IsVisible() { return m_EditFlags.is(flVisible); }
    virtual BOOL AllowMouseStart() = 0;

public:
    // modifiers
    shared_str m_ModifName;
    time_t m_ModifTime;
    // frame & Controls
    TUI_CustomControl* pCurControl;
    TForm* pFrame;

protected:
    void AddControl(TUI_CustomControl* c);
    TUI_CustomControl* FindControl(int subtarget, int action);
    void UpdateControl();

public:
    void SetAction(int action);
    void SetSubTarget(int target);
    void ResetSubTarget();

protected:
    void CreateDefaultControls(u32 sub_target_id);
    virtual void CreateControls() = 0;
    virtual void RemoveControls();

public:
    virtual void OnActivate();
    virtual void OnDeactivate();

    virtual void OnObjectsUpdate() { ; }
public:
    PropertyGP(FClassID, FClassID) ObjClassID ClassID;
    // definition
    virtual LPCSTR ClassName() = 0;
    virtual LPCSTR ClassDesc() = 0;
    virtual int RenderPriority() = 0;

public:
    ESceneToolBase(ObjClassID cls);
    virtual ~ESceneToolBase();

    virtual void OnCreate();
    virtual void OnDestroy();

    virtual bool AllowEnabling() = 0;
    // snap
    virtual ObjectList* GetSnapList() = 0;
    virtual void UpdateSnapList() = 0;

    // selection manipulate
    // flags: [0 - FALSE, 1 - TRUE, -1 - INVERT]
    virtual int RaySelect(
        int flag, float& distance, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly) = 0;
    virtual int FrustumSelect(int flag, const CFrustum& frustum) = 0;
    virtual void SelectObjects(bool flag) = 0;
    virtual void InvertSelection() = 0;
    virtual void RemoveSelection() = 0;
    virtual int SelectionCount(bool testflag) = 0;
    virtual void ShowObjects(bool flag, bool bAllowSelectionFlag = false, bool bSelFlag = true) = 0;

    virtual void Clear(bool bSpecific = false) = 0;
    virtual void Reset();

    // validation
    virtual bool Valid() = 0;
    virtual bool Validate(bool) = 0;

    // events
    virtual void OnDeviceCreate() = 0;
    virtual void OnDeviceDestroy() = 0;
    virtual void OnSynchronize() = 0;

    virtual void OnSceneUpdate() { ; }
    virtual void OnObjectRemove(CCustomObject* O, bool bDeleting) = 0;

    virtual void OnBeforeObjectChange(CCustomObject* O){};

    virtual void OnFrame() = 0;

    // render
    virtual void BeforeRender() { ; }
    virtual void OnRender(int priority, bool strictB2F) = 0;

    void OnRenderRoot(int priority, bool strictB2F)
    {
        if (IsVisible())
            OnRender(priority, strictB2F);
    };

    virtual void AfterRender() { ; }
    // IO
    virtual int SaveFileCount() const { return 1; }
    virtual bool IsNeedSave() = 0;

    virtual bool LoadStream(IReader&) = 0;
    virtual bool LoadLTX(CInifile&) = 0;
    virtual void SaveStream(IWriter&) = 0;
    virtual void SaveLTX(CInifile&, int id) = 0;

    virtual bool can_use_inifile() { return true; }
    virtual bool LoadSelection(IReader&) = 0;
    virtual void SaveSelection(IWriter&) = 0;

    virtual bool Export(LPCSTR path) { return true; }
    virtual bool ExportGame(SExportStreams* F) { return true; }
    virtual bool ExportStatic(SceneBuilder* B, bool b_selected_only) { return true; }
    virtual void GetStaticDesc(int& v_cnt, int& f_cnt, bool b_selected_only, bool b_cform) {}
    virtual bool GetStaticCformData(mesh_build_data& data, bool b_selected_only)
    {
#ifdef DEBUG
        int cnt_v = 0, cnt_f = 0;
        GetStaticDesc(cnt_v, cnt_f, b_selected_only, true);
        VERIFY(cnt_v == 0 && cnt_f == 0);
#endif
        return true;
    }

    virtual void CompileStaticStart(){};

    virtual void CompileStaticEnd(){};

    // properties
    virtual void FillProp(LPCSTR pref, PropItemVec& items) = 0;

    // utils
    virtual bool GetSummaryInfo(SSceneSummary* inf) = 0;

    virtual void HighlightTexture(LPCSTR tex_name, bool allow_ratio, u32 t_width, u32 t_height, BOOL mark) {}
    virtual void GetBBox(Fbox& bb, bool bSelOnly) = 0;

    virtual const CCustomObject* LastSelected() const { return NULL; }
};

DEFINE_MAP(ObjClassID, ESceneToolBase*, SceneToolsMap, SceneToolsMapPairIt);
#endif // ESceneCustomMToolsH
