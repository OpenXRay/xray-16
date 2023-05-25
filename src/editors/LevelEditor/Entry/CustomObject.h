#pragma once

#define CHUNK_OBJECT_BODY 0x7777

struct SRayPickInfo;
struct FSChunkDef;
class CFrustum;
class IReader;
class IWriter;
class COMotion;
class CCustomMotion;
class SAnimParams;
struct SSceneSummary;
class ESceneCustomOTool;

struct SExportStreamItem
{
    int chunk;
    CMemoryWriter stream;
    SExportStreamItem() : chunk(0) { ; }
};

struct SExportStreams
{
    SExportStreamItem spawn;
    SExportStreamItem spawn_rs;
    SExportStreamItem patrolpath;
    SExportStreamItem rpoint;
    SExportStreamItem sound_static;
    SExportStreamItem sound_env_geom;
    SExportStreamItem pe_static;
    SExportStreamItem envmodif;
    SExportStreamItem fog_vol;
};

class CCustomObject : private pureDrawUI
{
protected:
    shared_str EName;

private:
    // orientation
    Fvector EPosition;
    Fvector EScale;
    Fvector ERotation;
    SAnimParams *m_MotionParams;
    COMotion *m_Motion;

    // private animation methods
    void AnimationOnFrame();
    void AnimationDrawPath();
    void AnimationCreateKey(float t);
    void AnimationDeleteKey(float t);
    void AnimationUpdate(float t);

public:
    enum
    {
        flSelected_notused = (1 << 0),
        flVisible_notused = (1 << 1),
        flLocked_notused = (1 << 2),
        flMotion = (1 << 3),
        flRenderAnyWayIfSelected = (1 << 4),
        flObjectInGroup = (1 << 5),
        flObjectInGroupUnique = (1 << 6),

        flAutoKey = (1 << 30),
        flCameraView = (1 << 31),
    };

    Flags32 m_CO_Flags;

    enum
    {
        flRT_Valid = (1 << 0),
        flRT_UpdateTransform = (1 << 1),
        flRT_NeedSelfDelete = (1 << 2),
        flRT_Selected = (1 << 3),
        flRT_Visible = (1 << 4),
        flRT_SelectedLast = (1 << 5)
    };

    Flags32 m_RT_Flags;

public:
    shared_str FName;
    int save_id;
    // orientation
    Fvector FPosition;
    Fvector FScale;
    Fvector FRotation;
    Fmatrix FTransformP;
    Fmatrix FTransformR;
    Fmatrix FTransformS;
    Fmatrix FTransformRP;
    Fmatrix FTransform;
    Fmatrix FITransformRP;
    Fmatrix FITransform;

    CCustomObject *m_pOwnerObject;
    bool OnObjectNameAfterEdit(PropValue *sender, shared_str &edit_val);
    void OnTransformChange(PropValue *value);
    void OnMotionableChange(PropValue *sender);
    void OnMotionCommandsClick(ButtonValue *value, bool &bModif, bool &bSafe);
    void OnMotionFilesClick(ButtonValue *value, bool &bModif, bool &bSafe);
    void OnMotionControlClick(ButtonValue *value, bool &bModif, bool &bSafe);
    void OnMotionFrameChange(PropValue *value);
    void OnMotionKeyTimeChange(PropValue *value);

    void OnMotionCurrentFrameChange(PropValue *value);
    void OnMotionCameraViewChange(PropValue *value);

public:
    LPCSTR GetName() const { return *FName; }
    void SetName(LPCSTR N)
    {
        string256 tmp;
        strcpy(tmp, N);
        strlwr(tmp);
        FName = tmp;
    }

    virtual const Fvector &GetPosition() const { return FPosition; }
    virtual const Fvector &GetRotation() const { return FRotation; }
    virtual const Fvector &GetScale() const { return FScale; }

    virtual void SetPosition(const Fvector &pos)
    {
        FPosition.set(pos);
        UpdateTransform();
    }
    virtual void SetRotation(const Fvector &rot)
    {
        FRotation.set(rot);
        VERIFY(_valid(FRotation));
        UpdateTransform();
    }
    virtual void SetScale(const Fvector &scale)
    {
        FScale.set(scale);
        UpdateTransform();
    }

    void OnNameChange(PropValue *sender);
    void OnChangeIngroupUnique(PropValue *sender);

    void OnNumChangePosition(PropValue *sender);
    void OnNumChangeRotation(PropValue *sender);
    void OnNumChangeScale(PropValue *sender);

    virtual void DeleteThis() { m_RT_Flags.set(flRT_NeedSelfDelete, TRUE); }

public:
    CCustomObject(LPVOID data, LPCSTR name);
    virtual ~CCustomObject();

    BOOL Editable() const;

    IC BOOL Motionable() const { return m_CO_Flags.is(flMotion); }
    IC BOOL Visible() const { return m_RT_Flags.is(flRT_Visible); }
    IC BOOL Selected() const { return m_RT_Flags.is(flRT_Selected); }
    IC BOOL Valid() const { return m_RT_Flags.is(flRT_Valid); }
    IC BOOL IsDeleted() const { return m_RT_Flags.is(flRT_NeedSelfDelete); }

    // editor integration
    virtual bool Validate(bool bMsg) { return true; }
    virtual void FillProp(LPCSTR pref, PropItemVec &items);
    void AnimationFillProp(LPCSTR pref, PropItemVec &items);
    virtual bool GetSummaryInfo(SSceneSummary *inf);

    virtual void Select(int flag);
    virtual void Show(BOOL flag);
    void SetValid(BOOL flag) { m_RT_Flags.set(flRT_Valid, flag); }
    void SetRenderIfSelected(BOOL flag) { m_CO_Flags.set(flRenderAnyWayIfSelected, flag); }

    virtual bool IsRender();
    virtual void Render(int priority, bool strictB2F);
    void RenderRoot(int priority, bool strictB2F);
    virtual void OnFrame();
    virtual void OnUpdateTransform();

    virtual void OnSceneRemove(){};

    virtual bool RaySelect(int flag, const Fvector &start, const Fvector &dir, bool bRayTest = false); // flag 1,0,-1 (-1 invert)
    virtual bool FrustumSelect(int flag, const CFrustum &frustum);
    virtual bool RayPick(float &dist, const Fvector &start, const Fvector &dir, SRayPickInfo *pinf = NULL) { return false; };
    virtual bool FrustumPick(const CFrustum &frustum) { return false; };
    virtual bool SpherePick(const Fvector &center, float radius) { return false; };

    void ResetTransform()
    {
        FScale.set(1, 1, 1);
        FRotation.set(0, 0, 0);
        FPosition.set(0, 0, 0);
        FTransform.identity();
        FTransformRP.identity();
        FITransform.identity();
        FITransformRP.identity();
    }
    virtual void ResetAnimation(bool upd_t = true) { ; }
    virtual void UpdateTransform(bool bForced = false)
    {
        m_RT_Flags.set(flRT_UpdateTransform, TRUE);
        if (bForced)
            OnUpdateTransform();
    }

    // animation methods

    // grouping methods
    virtual void OnDetach();
    virtual void OnAttach(CCustomObject *owner);
    CCustomObject *GetOwner() { return m_pOwnerObject; }
    virtual bool CanAttach() = 0;

    virtual bool OnChooseQuery(LPCSTR specific) { return true; }

    // change position/orientation methods
    virtual void NumSetPosition(const Fvector &pos) { SetPosition(pos); }
    virtual void NumSetRotation(const Fvector &rot) { SetRotation(rot); }
    virtual void NumSetScale(const Fvector &scale) { SetScale(scale); }
    virtual void MoveTo(const Fvector &pos, const Fvector &up);
    virtual void Move(Fvector &amount);
    virtual void RotateParent(Fvector &axis, float angle);
    virtual void RotateLocal(Fvector &axis, float angle);
    virtual void RotatePivot(const Fmatrix &prev_inv, const Fmatrix &current);
    virtual void Scale(Fvector &amount);
    virtual void ScalePivot(const Fmatrix &prev_inv, const Fmatrix &current, Fvector &amount);

    virtual bool LoadStream(IReader &);
    virtual bool LoadLTX(CInifile &ini, LPCSTR sect_name);
    virtual void SaveStream(IWriter &);
    virtual void SaveLTX(CInifile &ini, LPCSTR sect_name);

    virtual bool ExportGame(SExportStreams *data) { return true; }

    virtual bool GetBox(Fbox &box) { return false; }
    virtual bool GetUTBox(Fbox &box) { return false; }
    virtual void OnSceneUpdate() { ; }
    virtual void OnObjectRemove(const CCustomObject *object) { ; }
    virtual bool OnSelectionRemove() { return true; } // ���������� ����� �� ��� ������� ������

    virtual void OnDeviceCreate() { ; }
    virtual void OnDeviceDestroy() { ; }

    virtual void OnSynchronize();
    virtual void OnShowHint(AStringVec &dest);

    virtual LPCSTR RefName() { return 0; }

    IC const Fmatrix &_ITransform() { return FITransform; }
    IC const Fmatrix &_Transform() { return FTransform; }
    IC const Fvector &_Position() { return FPosition; }
    IC const Fvector &_Rotation() { return FRotation; }
    IC const Fvector &_Scale() { return FScale; }

    ObjClassID FClassID;
    ESceneCustomOTool *FParentTools;

public:
    static void SnapMove(Fvector &pos, Fvector &rot, const Fmatrix &rotRP, const Fvector &amount);
    static void NormalAlign(Fvector &rot, const Fvector &up, const Fvector &dir);

private:
    virtual void OnDrawUI();

private:
    int m_ButtonId;
    float m_FromTime;
    float m_ToTime;
    float m_ScaleFactor;
    float m_Speed;
};
